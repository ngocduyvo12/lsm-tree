#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
    // constructor that launches a given number of threads
    ThreadPool(size_t threads);

    // function template to add a new task to the pool
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;

    // destructor that joins all threads
    ~ThreadPool();

protected:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;

private:
    // the task queue
    std::queue< std::function<void()> > tasks;

    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// constructor that launches a given number of threads
inline ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this]
            {
                for(;;)
                {
                    std::function<void()> task;

                    {
                        // lock the queue to get a task from it
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        
                        // wait for a task to be added to the queue or the pool to be stopped
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->tasks.empty(); });
                        
                        // if the pool is stopped and the queue is empty, the thread can exit
                        if(this->stop && this->tasks.empty())
                            return;
                        
                        // get a task from the queue
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }

                    // execute the task
                    task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    // define the return type based on the function signature
    using return_type = typename std::result_of<F(Args...)>::type;

    // create a packaged task that wraps the function and its arguments
    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    // get a future that will eventually hold the result of the task
    std::future<return_type> res = task->get_future();

    {
        // lock the queue while adding a new task
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        // add the new task to the queue as a lambda function that invokes the packaged task
        tasks.emplace([task](){ (*task)(); });
    }

    // notify one waiting thread that there is work to be done
    condition.notify_one();
    
    // return the future that holds the result of the task
    return res;
}

// This is the destructor of the ThreadPool class, which is called when the object is destroyed.
inline ThreadPool::~ThreadPool()
{
    // Enter a critical section by acquiring a lock on the queue_mutex object
    // and set the stop flag to true, which signals the threads to stop processing tasks.
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    // Notify all waiting threads that the condition variable has changed, which is used
    // to wake up any threads waiting on this condition. In this case, they will wake up
    // and check the stop flag to see if they should stop processing tasks.
    condition.notify_all();

    // Join all threads in the workers vector, blocking until all threads have completed their work.
    // This ensures that the object is properly cleaned up and all resources are released.
    for(std::thread &worker: workers)
        worker.join();
}

#endif
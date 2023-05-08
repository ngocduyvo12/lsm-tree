#include <functional>
#include <vector>

#include "thread_pool.h"

using namespace std;

// Define a type alias for a worker_task, which is a function taking no arguments and returning void.
typedef function<void()> worker_task;

// WorkerPool is a class that inherits from ThreadPool.
// It is used to manage a pool of worker threads and provides methods to launch tasks and wait for their completion.
class WorkerPool : ThreadPool {
    // A vector to store future objects returned by tasks launched on the worker threads.
    vector<future<void>> futures;

public:
    // Inherit ThreadPool constructors.
    using ThreadPool::ThreadPool;

    // Launches the given worker_task on all worker threads in the pool.
    void launch(worker_task&);

    // Waits for all tasks in the 'futures' vector to complete and then clears the 'futures' vector.
    void wait_all(void);
};

// DynamicWorkerPool is a class used to manage a pool of dynamic worker threads.
// It creates new threads when tasks are launched and provides methods to wait for their completion.
class DynamicWorkerPool {
    // A vector to store thread objects representing the worker threads.
    vector<thread> workers;

public:
    // Launches the given worker_task on all worker threads in the pool by creating new threads for each worker.
    void launch(worker_task&);

    // Waits for all worker threads to complete their tasks and join the main thread.
    void wait_all(void);
};

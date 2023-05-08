#include <cassert>

#include "worker_pool.h"

// This method launches a worker_task on all workers in the pool
// and stores the future objects in the 'futures' vector.
void WorkerPool::launch(worker_task& task) {
    // Make sure the 'futures' vector is empty before launching tasks.
    assert(futures.size() == 0);

    // For each worker in the pool, enqueue the task and store the future object.
    for (int i = 0; i < workers.size(); i++) {
        futures.push_back(enqueue(task));
    }
}

// This method waits for all tasks in the 'futures' vector to complete
// and clears the 'futures' vector.
void WorkerPool::wait_all(void) {
    // Wait for each future object to complete.
    for (auto& future : futures) {
        future.wait();
    }

    // Clear the 'futures' vector.
    futures.clear();
}

// This method launches a worker_task on all workers in the DynamicWorkerPool
// by creating a new thread for each worker and passing the task to it.
void DynamicWorkerPool::launch(worker_task& task) {
    // For each worker in the pool, create a new thread and pass the task.
    for (int i = 0; i < workers.size(); i++) {
        workers.emplace_back(task);
    }
}

// This method waits for all worker threads to complete their tasks
// and join the main thread.
void DynamicWorkerPool::wait_all(void) {
    // Wait for each worker thread to complete and join the main thread.
    for (auto& worker : workers) {
        worker.join();
    }
}
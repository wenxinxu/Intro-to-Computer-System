/**
 * File: thread-pool.h
 * -------------------
 * This class defines the ThreadPool class, which accepts a collection
 * of thunks (which are zero-argument functions that don't return a value)
 * and schedules them in a FIFO manner to be executed by a constant number
 * of child threads that exist solely to invoke previously scheduled thunks.
 */

#ifndef _thread_pool_
#define _thread_pool_

#include <cstddef>     // for size_t
#include <functional>  // for the function template used in the schedule signature
#include <thread>      // for thread
#include <vector>      // for vector
#include <semaphore.h> // for semaphore
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>
#include "ostreamlock.h"
class ThreadPool {
public:

    /* Constructs a ThreadPool configured to spawn up to the specified
     * number of threads. */
    ThreadPool(size_t numThreads);

    /* Schedules the provided thunk (which is something that can
     * be invoked as a zero-argument function without a return value)
     * to be executed by one of the ThreadPool's threads as soon as
     * all previously scheduled thunks have been handled. */
    void schedule(const std::function<void(void)>& thunk);

    void dispatcher(); // signaled by worker and by scheduler

    void worker(size_t id); //signaled by dispatcher

    /* Blocks and waits until all previously scheduled thunks
     * have been executed in full. */
    void wait();

    /* Waits for all previously scheduled thunks to execute, and then
     * properly brings down the ThreadPool and any resources tapped
     * over the course of its lifetime. */
    ~ThreadPool();
  
private:
    std::thread dt;                // dispatcher thread handle
    std::vector<std::thread> wts;  // worker thread handles. you may want to change/remove this
    semaphore hasJob; //  dispatcher signals worker that a job needs to be done
    semaphore jobReady; // schedulor signals dispatcher
    semaphore hasWorker; // worker signals dispatcher that a worker is available
    std::condition_variable cv; // for wait() function waiting jobqueue empty
    std::queue<std::function<void(void)>> readyQueue; // schedulor enqueue, dispatcher dequeue
    std::queue<std::function<void(void)>> jobQueue; // dispatcher enqueue and worker dequeue;
    size_t jobNum;

    std::mutex qlock; //queue lock
    std::mutex jobNumLock; //lock total number of job
    std::mutex lk;



    /* ThreadPools are the type of thing that shouldn't be cloneable, since it's
     * not clear what it means to clone a ThreadPool (should copies of all outstanding
     * functions to be executed be copied?).
     *
     * In order to prevent cloning, we remove the copy constructor and the
     * assignment operator.  By doing so, the compiler will ensure we never clone
     * a ThreadPool. */
    ThreadPool(const ThreadPool& original) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
};

#endif

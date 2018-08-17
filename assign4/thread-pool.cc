/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), hasJob(0) ,jobReady(0),hasWorker(numThreads), jobNum(0)
 {
    dt = thread([this]() {
        dispatcher();
    });
    for(size_t i = 0; i < numThreads; i++) {
        wts[i] = thread([this,i]() {
            worker(i);
        });
    }

}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    jobNumLock.lock();
    jobNum++;
    jobNumLock.unlock();
    qlock.lock();
    readyQueue.push(thunk);
    qlock.unlock();
    jobReady.signal();


}

void ThreadPool::wait() {
    std::unique_lock<mutex> lg(lk);
    cv.wait(lg,[this]{
        return jobNum == 0;
    });

}
void ThreadPool::worker(size_t id) {

    while (true) {
        hasJob.wait();
        qlock.lock();
        if(jobQueue.empty()) {
            //no job is comming
            qlock.unlock();
            break;
        } else {
            function<void(void)> f = jobQueue.front();
            jobQueue.pop();
            qlock.unlock();
            f();
            hasWorker.signal();
            jobNumLock.lock();
            jobNum--; // one more job is done, decrement total
            jobNumLock.unlock();
            if(jobNum == 0)cv.notify_all();  //tell wait() to wake up and check if total job has dropped to 0
        }
    }

}
void ThreadPool::dispatcher() {
    while(true) {
        jobReady.wait();
        hasWorker.wait();

        qlock.lock();
        if(readyQueue.empty()) {
            //there is not job assigned by schedulor
            qlock.unlock();
            break;
        }
        else {
            function<void(void)> f = readyQueue.front();
            readyQueue.pop();
            jobQueue.push(f);
            qlock.unlock();
            hasJob.signal();

        }
    }


}
ThreadPool::~ThreadPool() {
    wait();
    for(size_t i = 0; i < wts.size(); i++ ) {
        hasJob.signal();
    }
    jobReady.signal();
    dt.join();
    for(size_t i = 0; i < wts.size(); i++ ) {
        wts[i].join();
    }

}

#ifndef QUEUE_H
#define QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class Queue {
  public:

    /**
     * Pop a value from the queue.
     *
     * Take the lock and wait on the conditional variable
     * till a notification comes of data in the queuea
     *
     * Return the popped value
     */
    T pop() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(mlock);
        }
        auto val = queue_.front();
        queue_.pop();
        return val;
    }

    void pop(T& item) {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (queue_.empty()) {
            cond_.wait(mlock);
        }
        item = queue_.front();
        queue_.pop();
    }

    /**
     * Push a new value into the queue
     *
     * Take a lock and push the new value
     *
     * Wake up the conditional variable to allow
     * pop of new value
     */
    void push(const T& item) {
        std::unique_lock<std::mutex> mlock(mutex_);
            queue_.push(item);
        mlock.unlock();
        cond_.notify_one();
    }
    
    /**
     * Take a lock and clear all values from the
     * queue
     */
    void clear() {
        std::unique_lock<std::mutex> mlock(mutex_);
        while (!queue_.empty())
            pop();
        mlock.unlock();
        cond_.notify_all();
    }

    Queue()=default;
    Queue(const Queue&) = delete;            // disable copying
    Queue& operator=(const Queue&) = delete; // disable assignment
  
 private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif

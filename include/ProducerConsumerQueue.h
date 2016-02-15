#ifndef PRODUCER_CONSUMER_QUEUE_H_
#define PRODUCER_CONSUMER_QUEUE_H_

#include <cstdatomic>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace distdpi {

template<class T>
struct ProducerConsumerQueue {
  typedef T value_type;

    ProducerConsumerQueue(const ProducerConsumerQueue&) = delete;
    ProducerConsumerQueue& operator = (const ProducerConsumerQueue&) = delete;

    explicit ProducerConsumerQueue(uint32_t size)
        : size_(size)
        , records_(static_cast<T*>(std::malloc(sizeof(T) * size)))
        , readIndex_(0)
        , writeIndex_(0)
    {
        assert(size >= 2);
        if (!records_) {
            throw std::bad_alloc();
        }
    }

    ~ProducerConsumerQueue() {
        size_t read = readIndex_;
        size_t end = writeIndex_;
        while (read != end) {
            records_[read].~T();
            if (++read == size_) {
            read = 0;
            }
        }

        std::free(records_);
    }

    template<class ...Args>
    bool write(Args&&... recordArgs) {
        auto const currentWrite = writeIndex_.load(std::memory_order_relaxed);
        auto nextRecord = currentWrite + 1;
        if (nextRecord == size_) {
            nextRecord = 0;
        }
        if (nextRecord != readIndex_.load(std::memory_order_acquire)) {
            new (&records_[currentWrite]) T(std::forward<Args>(recordArgs)...);
            writeIndex_.store(nextRecord, std::memory_order_release);
            return true;
        }

        // queue is full
        return false;
    }

    bool read(T& record) {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
            // queue is empty
            return false;
        }

        auto nextRecord = currentRead + 1;
        if (nextRecord == size_) {
            nextRecord = 0;
        }
        record = std::move(records_[currentRead]);
        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);
        return true;
    }

    T* frontPtr() {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        if (currentRead == writeIndex_.load(std::memory_order_acquire)) {
            // queue is empty
            return NULL;
        }
        return &records_[currentRead];
    }

    void popFront() {
        auto const currentRead = readIndex_.load(std::memory_order_relaxed);
        assert(currentRead != writeIndex_.load(std::memory_order_acquire));

        auto nextRecord = currentRead + 1;
        if (nextRecord == size_) {
            nextRecord = 0;
        }
        records_[currentRead].~T();
        readIndex_.store(nextRecord, std::memory_order_release);
    }

    bool isEmpty() const {
        return readIndex_.load(std::memory_order_consume) ==
            writeIndex_.load(std::memory_order_consume);
    }

    bool isFull() const {
        auto nextRecord = writeIndex_.load(std::memory_order_consume) + 1;
        if (nextRecord == size_) {
            nextRecord = 0;
        }
        if (nextRecord != readIndex_.load(std::memory_order_consume)) {
            return false;
        }
        // queue is full
        return true;
    }

    size_t sizeGuess() const {
        int ret = writeIndex_.load(std::memory_order_consume) -
                  readIndex_.load(std::memory_order_consume);
        if (ret < 0) {
            ret += size_;
        }
        return ret;
    }

  private:
    const uint32_t size_;
    T* const records_;

    std::atomic<unsigned int> readIndex_;
    std::atomic<unsigned int> writeIndex_;
};

}

#endif

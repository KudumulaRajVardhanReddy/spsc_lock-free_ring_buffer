//Standard mutex queue using locks
#pragma once

#include <queue>
#include <mutex>

template <typename T, size_t Capacity>
class StandardMutexQueue {
public:
    StandardMutexQueue() = default;

    StandardMutexQueue(const StandardMutexQueue&) = delete;
    StandardMutexQueue& operator=(const StandardMutexQueue&) = delete;

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size() >= Capacity) return false;

        queue_.push(item);
        return true;
    }

    bool pop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) return false;

        item = queue_.front();
        queue_.pop();

        return true;
    }

private:
    std::mutex mutex_;
    std::queue<T> queue_;
};

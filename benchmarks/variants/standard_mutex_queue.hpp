//Standard mutex queue using locks
#pragma once

#include <queue>
#include <mutex>
#include <optional>

template <typename T, size_t Capacity>
class StandardMutexQueue {
public:
    StandardMutexQueue() = default;

    bool push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.size >= Capacity) return false;

        queue_.push(item);
        return true;
    }

    std::optional<T> pop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty()) return std::nullopt;

        T item = queue_.front();
        queue_.pop();

        return item;
    }

private:
    std::mutex mutex_;
    std::queue<T> queue_;
};

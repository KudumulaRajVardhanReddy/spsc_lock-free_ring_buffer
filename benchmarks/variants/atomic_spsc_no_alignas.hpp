//Lock-Free SPSC Ring Buffer - No cache alignment

#pragma once

#include <optional>
#include <atomic>
#include <cstddef>

template<typename T, size_t Capacity>
class AtomicSPSCNoAlignas {
    static_assert(Capacity & (Capacity - 1) == 0 && Capacity > 0, "Capacity must be a power of 2.");
public:

    AtomicSPSCNoAlignas() : write_index_(0), read_index_(0) {}

    bool push(const T& item) {
        const size_t current_write = write_index_.load(std::memory_order_relaxed);
        const size_t current_read = read_index_.load(std::memory_order_acquire);

        if ((current_write + 1) & kMask == current_read) return false;

        buffer_[current_write & kMask] = item;
        write_index_.store(current_write + 1, std::memory_order_release);

        return true;
    }

    std::optional<T> pop() {
        const size_t current_read = read_index_.load(std::memory_order_relaxed);
        const size_t current_write = write_index_.load(std::memory_order_acquire);

        if (current_read == current_write) return std::nullopt;

        T item = buffer_[current_read & kMask];
        read_index_.store(current_read + 1, std::memory_order_release);

        return item;
    }

private:
    static constexpr size_t kMask = Capacity - 1;
    T buffer_[Capacity];
    std::atomic<size_t> write_index_;
    std::atomic<size_t> read_index_;
};

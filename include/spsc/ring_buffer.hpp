// Atomic SPSC Ring Buffer - Cache Alignment using alignas

#pragma once

#include <atomic>
#include <optional>
#include <cstddef>
#include <new>

template<typename T, size_t Capacity>
class SPSCRingBuffer {
    static_assert(Capacity & (Capacity - 1) == 0 && Capacity > 0, "Capacity must be a power of 2.");

public:
    SPSCRingBuffer() : write_index_(0), read_index_(0) {}

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

    constexpr size_t capacity() const { return Capacity; }

private:
    T buffer_[Capacity];
    static constexpr size_t kMask = Capacity - 1;

    alignas(64) std::atomic<size_t> write_index_;
    alignas(64) std::atomic<size_t> read_index_;
};

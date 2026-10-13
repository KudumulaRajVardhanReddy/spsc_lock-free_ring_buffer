// Atomic SPSC Ring Buffer - Cache Alignment using alignas

#pragma once

#include <new>
#include <atomic>
//#include <optional>  // Dropping optional for preventing copy overhead and stack wrapping!!
#include <cstddef>

namespace spsc {
    #ifdef __cpp_lib_hardware_interference_size
        // If the compiler supports this C++ feature, pulls this.
        using std::hardware_destructive_interference_size;
    #else
        // If the compiler doesn't support, it hardcodes 64 bytes cache-line size.
        constexpr std::size_t hardware_destructive_interference_size = 64;
    #endif
}

template<typename T, size_t Capacity>
class SPSCRingBuffer {
    static_assert(((Capacity & (Capacity - 1)) == 0) && Capacity > 0, "Capacity must be a power of 2.");

public:
    SPSCRingBuffer() : write_index_(0), read_index_(0) {}

    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;
    // disabling moving, copying avoids compiler mocing this object
    // outside the cache-aligned memory boundary.

    bool push(const T& item) {
        const size_t current_write = write_index_.load(std::memory_order_relaxed);
        const size_t current_read = read_index_.load(std::memory_order_acquire);

        if (current_write - current_read >= Capacity) return false;

        buffer_[current_write & kMask] = item;
        write_index_.store(current_write + 1, std::memory_order_release);

        return true;
    }

    /*
    std::optional<T> pop() {
        const size_t current_read = read_index_.load(std::memory_order_relaxed);
        const size_t current_write = write_index_.load(std::memory_order_acquire);

        if (current_read == current_write) return std::nullopt;

        T item = buffer_[current_read & kMask];
        read_index_.store(current_read + 1, std::memory_order_release);

        return item;
    }

    Replacing std::optional pop with Pass-by-Reference pop avoiding std::optional overhead
    */

    bool pop(T& item) {
        const size_t current_read = read_index_.load(std::memory_order_relaxed);
        const size_t current_write = write_index_.load(std::memory_order_acquire);

        if (current_read == current_write) return false;

        item = buffer_[current_read & kMask];
        read_index_.store(current_read + 1, std::memory_order_release);

        return true;
    }

    constexpr size_t capacity() const { return Capacity; }

private:
    static constexpr size_t kMask = Capacity - 1;

    /*
     Replacing hardcoded alignas(64) into alignas(spsc::hardware_destructive_interference_size)
     former assumes target hardware has 64 bytes cache-line, the later dynamically finds out
     the target hardware cache-line.
     */
    alignas(spsc::hardware_destructive_interference_size) std::atomic<size_t> write_index_;
    alignas(spsc::hardware_destructive_interference_size) std::atomic<size_t> read_index_;
    alignas(spsc::hardware_destructive_interference_size) T buffer_[Capacity];
};

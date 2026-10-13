# Lock-Free Single-Producer Single-Consumer Ring Buffer

A battle-tested, ultra-low-latency **Single-Producer Single-Consumer (SPSC)** lock-free ring buffer implemented in high-performance C++.

---

## Key Features

* **Zero Lock Contention:** Completely lock-free operations utilizing fine-grained atomic operations with optimal Acquire-Release memory ordering semantics.
* **Dynamic Hardware Cache Alignment:** Replaces hardcoded values with `alignas(spsc::hardware_destructive_interference_size)` to dynamically query the target architecture's cache line boundary, fully mitigating **False Sharing**.
* **Zero-Allocation Hot Paths:** Upgraded to an ultra-fast pass-by-reference signature (`bool pop(T& item)`), completely eliminating `std::optional` structural wrapping, copy overheads, and stack memory padding footprints.
* **Bounded Power-of-Two Capacity:** Enforces strict power-of-two capacity validation at compile time, enabling high-speed bitwise masking (`index & kMask`) for O(1) routing, entirely avoiding expensive integer modulo (`%`) instructions.
* **Compile-Time Shields:** Explicitly deletes copy constructors and assignment operators (`= delete`) at the compiler phase to prevent layout shifts or accidental object slicing in thread contexts.

---

## Technical Architecture

### Memory Order & Synchronization
* **Producer (`push`):** Loads the trailing consumer position using `std::memory_order_acquire` and publishes updates using `std::memory_order_release`.
* **Consumer (`pop`):** Reads the advancing producer position using `std::memory_order_acquire` and releases ownership updates via `std::memory_order_release`.

### Cache Line Layout
To achieve absolute mechanical sympathy, the internal memory sequence enforces strict compilation placement boundaries to isolate active thread pipelines onto distinct hardware cache lines:

+-------------------------------------------------------------+
| write_index_ (Producer)  | Dynamic Cache Padding            | -> Cache Line 0
+-------------------------------------------------------------+
| read_index_  (Consumer)  | Dynamic Cache Padding            | -> Cache Line 1
+-------------------------------------------------------------+
| T buffer_[Capacity]      | Core Storage Memory Blocks       | -> Cache Line 2+
+-------------------------------------------------------------+
---

## Performance Benchmarks

Evaluated under desynchronized processing conditions passing 1,000,000 data items through a `65536` capacity queue configuration:

* **CPU:** Intel Core i9-14900HX (24 Cores / 32 Threads, 2.4GHz Base / 5.8GHz Turbo)
* **Compiler:** MinGW GCC C++17 (`-O3 -march=native -Wall -Wextra -Wno-interference-size`)
* **OS:** Windows 11 (Thread Affinity mapped to separate physical Performance Cores)

| Implementation | Wall Clock Time | Execution Behavior | Speedup vs. Mutex |
| :--- | :--- | :--- | :--- |
| **`std::mutex` + `std::queue`** | ~142.00 ms | Throttled by OS scheduling context switches and lock contention. | Baseline |
| **Atomic SPSC (No `alignas`)** | ~6.39 ms | Delayed by cache line bouncing across shared variables. | ~22.2x |
| **Cache-Aligned `SPSCRingBuffer`** | **~3.52 ms** | **Parallel hardware execution with zero cache conflicts.** | **~40.3x** |

> **Note on Benchmark Methodology:**
> When threads operate under distinct processing speeds, the cache-aligned `SPSCRingBuffer` avoids cache validation stalls, achieving a **~45% speed advantage** over the unaligned queue variant by fully isolating indices onto distinct physical cache lines.

---

## Requirements & Building

### Prerequisites
* C++17 compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+)
* CMake 3.14 or higher
* Ninja or Make build tool

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com
   cd spsc_lock-free_ring_buffer
   ```

2. **Configure with CMake (Release Mode):**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "Ninja"
   ```

3. **Build the targets:**
   ```bash
   cmake --build build
   ```

4. **Run Unit and Concurrency Tests:**
   ```bash
   ./build/test_ring_buffer
   ```

5. **Run the Comparison Benchmarks:**
   ```bash
   ./build/bench_comparison
   ```

---

## What I Changed from previous iteration

I refactored the initial codebase to fix memory layout overhead, clear compilation bugs, and ensure the performance benchmarks are accurate and fair:

* **Ditched `std::optional`:** Changed `pop()` from returning a `std::optional<T>` to using a pass-by-reference signature `bool pop(T& item)`. This stops the compiler from padding the data structure and removes stack copying completely.
* **Smart Cache Alignment:** Replaced hardcoded `alignas(64)` markers with `alignas(spsc::hardware_destructive_interference_size)` so the compiler automatically handles the exact cache line limits of whatever target CPU it builds on.
* **Variable Reordering:** Swapped the private member declaration order to `write_index_` -> `read_index_` -> `buffer_`. This makes sure the producer and consumer indexes are strictly mapped onto separate physical cache lines.
* **Fixed Benchmark Deadlocks & Accuracy:**
  * Added missing `while` spin-retry loops to the `BM_StandardMutexQueue` producer and consumer. This prevents the producer from dropping items when the queue fills up, which previously left the consumer stuck in an infinite deadlock loop.
  * Moved the `StandardMutexQueue` instantiation inside the benchmark loop so each run starts with a completely fresh, empty queue instead of using polluted data from prior iterations.
  * Switched the `CMakeLists.txt` build configuration from global include paths to modern target-isolated paths (`target_include_directories`). This stops our custom flags from leaking into the Google Benchmark code and crashing the compiler.
* **Updated Tests:** Rewrote the Google Test suite files (`test_ring_buffer.cpp`) to handle the new reference parameters and boolean return values instead of calling old `.has_value()` methods.

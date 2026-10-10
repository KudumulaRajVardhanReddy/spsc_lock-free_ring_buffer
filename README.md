# Lock-Free Single-Producer Single-Consumer Ring Buffer

A **Single-Producer Single-Consumer (SPSC)** lock-free ring buffer implemented in C++.

---

## Key Features

* **Zero Lock Contention:** Completely lock-free operations utilizing fine-grained C++11 atomic operations with Acquire-Release memory ordering semantics.
* **Cache-Line Isolation:** Producer (`write_index_`) and Consumer (`read_index_`) indices are explicitly aligned to 64-byte boundaries using `alignas(64)` to eliminate false sharing across CPU L1 cache lines.
* **Bounded Power-of-Two Capacity:** Employs bitwise AND masking (`index & (Capacity - 1)`) for $O(1)$ array indexing, avoiding expensive integer modulo operations.
* **Monotonic Counter Architecture:** Prevents overflow and edge-case wrapping issues through unbounded `uint64_t` index tracking.
* **Header-Only:** Zero external binary dependencies; ready to drop into any C++17 project.

---

## Technical Architecture

### Memory Order & Synchronization
* **Producer (`emplace` / `push`):** Reads the consumer index with `std::memory_order_relaxed` (or `acquire` when updating cached bounds) and writes the published index with `std::memory_order_release`.
* **Consumer (`pop` / `front`):** Reads the producer index with `std::memory_order_acquire` and updates the consumer index with `std::memory_order_release`.

### Cache Line Layout
To eliminate false sharing, the internal memory layout isolates thread-specific atomic fields:

```
+-------------------------------------------------------------+
| write_index_ (Producer)  | 64-byte alignas padding          | -> Cache Line 0
+-------------------------------------------------------------+
| read_index_  (Consumer)  | 64-byte alignas padding          | -> Cache Line 1
+-------------------------------------------------------------+
| Buffer Storage Pointer / Array                              | -> Cache Line 2
+-------------------------------------------------------------+
```

---

## Performance Benchmarks

Evaluated on x86_64 architecture using Google Benchmark (1,000,000 operations per iteration in Release build `-O3`):

| Implementation | Execution Time | Throughput (Ops/sec) | Speedup vs. Baseline |
| :--- | :--- | :--- | :--- |
| **`std::mutex` + Queue** | ~55.40 ms | ~18.05 M ops/sec | Baseline |
| **Atomic SPSC (No `alignas`)** | ~1.80 ms | ~555.55 M ops/sec | ~30.7x |
| **Cache-Aligned SPSCRingBuffer** | **~2.68 ms** | **~373.13 M ops/sec** | **~20.6x** |

> **Why is unaligned slightly faster in this microbenchmark?**
> In isolated synthetic tests without outer application work, unaligned atomic indices share a single L1 cache line, allowing tight-loop execution on hot hardware prefetchers. 
> However, `alignas(64)` is used in production to enforce strict cache-line isolation. This prevents **false sharing** (cache-line bouncing) when producer and consumer threads run concurrently alongside real application workloads on separate CPU cores.

> *Note: Raw Google Benchmark metrics are captured in `benchmarks/release.json`.*

---

## Requirements & Building

### Prerequisites
* C++17 compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+)
* CMake 3.15 or higher
* Ninja or Make build tool

### Build Instructions

1. **Clone the repository:**
   ```bash
   git clone https://github.com/KudumulaRajVardhanReddy/spsc_lock-free_ring_buffer.git
   cd spsc_lock-free_ring_buffer
   ```

2. **Configure with CMake (Release Mode):**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G "Ninja"
   ```

3. **Build the project:**
   ```bash
   cmake --build build
   ```

4. **Run Unit Tests:**
   ```bash
   ./build/test_ring_buffer
   ```

5. **Run Benchmarks:**
   ```bash
   ./build/bench_comparison
   ```

---

## Future Development & Maintenance

The ring buffer is currently implemented in C++17. I plan to continue maintaining the project and progressively bring it up to C++20 and C++23, updating the implementation as newer language features become part of the codebase.

Future revisions will also cover the underlying implementation, memory ordering, cache behavior, and benchmarking methodology. Changes will be accompanied by tests and performance measurements to verify their impact, with previous design decisions revisited as the implementation evolves.

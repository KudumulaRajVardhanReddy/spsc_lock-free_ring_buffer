//benchmarking - Performance Comparison

#include <benchmark/benchmark.h>
#include <thread>
#include "variants/standard_mutex_queue.hpp"
#include "variants/atomic_spsc_no_alignas.hpp"
#include "spsc/ring_buffer.hpp"

constexpr size_t kCapacity = 1024;

//Benchmarks for Standard Mutex Queue
static void BM_StandardMutexQueue(bench::State& state) {
    StandardMutexQueue<size_t> queue;
    const size_t num_items = state.range(0);

    for (auto _ : state) {
        std::thread producer([&]() {
            for (size_t i = 0; i < num_items; ++i) queue.push(i);
        });

        std::thread consumer([&]() {
            for (size_t i = 0; i < num_items; ++i) queue.pop();
        });

        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * num_items * 2);
}

//Benchmarks for Atomic SPSC Ring Buffer without Cache Alignment
static void BM_AtomicSPSCNoAlignas(benchmark::State& state) {
    const size_t num_items = state.range(0);

    for (auto _ : state) {
        AtomicSPSCNoAlignas<size_t, kCapacity> buffer;

        std::thread producer([&]() {
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.push(i)) std::thread::yield();
            }
        });

        std::thread consumer([&]() {
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.pop().has_value()) std::this_thread::yield();
            }
        });

        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * num_items * 2);
}

//Benchmarks for full fledged SPSC Ring Buffer with Cache Alignment
static void BM_SPSCRingBuffer(benchmark::State& state) {
    const size_t num_items = state.range(0);

    for (auto _ : state) {
        SPSCRingBuffer<size_t, kCapacity> buffer;

        std::thread producer([&]() {
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.push(i)) std::this_thread::yield();
            }
        });

        std::thread consumer([&]() {
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.pop()) std::this_thread::yield();
            }
        });

        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * num_items * 2);
}

BENCHMARK(BM_StandardMutexQueue)->Arg(1000000)->Unit(benchmark::kMilliseconds);
BENCHMARK(BM_AtomicSPSCNoAlignas)->Arg(1000000)->Unit(benchmark::kMilliseconds);
BENCHMARK(BM_SPSCRingBuffer)->Arg(1000000)->Unit(benchmark::kMilliseconds);

BENCHMARK_MAIN();

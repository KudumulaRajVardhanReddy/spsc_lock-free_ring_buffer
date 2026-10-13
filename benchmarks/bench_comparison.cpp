// Benchmarking - Performance Comparison

#include <benchmark/benchmark.h>
#include <thread>

// Implementing Thread Affinity(Core Pinning).
#if defined(__linux__)
    #include <pthread.h>
    #include <sched.h>
#elif defined(_WIN32)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <pthread.h>
    #include <mach/thread_act.h>
    #include <mach/thread_policy.h>
#endif

#include "variants/standard_mutex_queue.hpp"
#include "variants/atomic_spsc_no_alignas.hpp"
#include "spsc/ring_buffer.hpp"

constexpr size_t kCapacity = 1024;

inline void pin_thread_to_core(int core_or_tag) {
    #if defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_or_tag, &cpuset);
        pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    #elif defined(_WIN32)
        if (core_or_tag >= 0 && core_or_tag < 64) {
            SetThreadAffinityMask(GetCurrentThread(), 1ULL << core_or_tag);
        }
    #elif defined(__APPLE__)
        thread_affinity_policy_data_t policy = { core_or_tag };
        thread_port_t mach_thread = pthread_mach_thread_np(pthread_self());
        thread_policy_set(mach_thread,
            THREAD_AFFINITY_POLICY,
            (thread_policy_t)&policy,
            THREAD_AFFINITY_POLICY_COUNT);
    #endif
}

//Benchmarks for Standard Mutex Queue
static void BM_StandardMutexQueue(benchmark::State& state) {

    const size_t num_items = state.range(0);

    for (auto _ : state) {

        StandardMutexQueue<size_t, kCapacity> queue;

        std::thread producer([&]() {
            pin_thread_to_core(2);
            for (size_t i = 0; i < num_items; ++i) queue.push(i);
        });

        std::thread consumer([&]() {
            pin_thread_to_core(4);

            size_t item = 0;
            for (size_t i = 0; i < num_items; ++i) {
                while (!queue.pop(item)) std::this_thread::yield();
            }
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
            pin_thread_to_core(2);
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.push(i)) std::this_thread::yield();
            }
        });

        std::thread consumer([&]() {
            pin_thread_to_core(4);

            size_t item = 0;
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.pop(item)) std::this_thread::yield();
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
            pin_thread_to_core(2);
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.push(i)) std::this_thread::yield();
            }
        });

        std::thread consumer([&]() {
            pin_thread_to_core(4);

            size_t item = 0;
            for (size_t i = 0; i < num_items; ++i) {
                while (!buffer.pop(item)) std::this_thread::yield();
            }
        });

        producer.join();
        consumer.join();
    }
    state.SetItemsProcessed(state.iterations() * num_items * 2);
}

BENCHMARK(BM_StandardMutexQueue)->Arg(1000000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_AtomicSPSCNoAlignas)->Arg(1000000)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_SPSCRingBuffer)->Arg(1000000)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();

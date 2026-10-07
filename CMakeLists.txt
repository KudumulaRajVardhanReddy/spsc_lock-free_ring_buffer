cmake_minimum_required(VERSION 3.14)
project(SPSCRingBuffer CXX

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/02 /arch:AVX2)
else()
    add
endif()

include_directories(include benchmarks)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

enable_testing()
add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
target_link_libraries(test_ring_buffer PRIVATE gtest_main)

add_executable(bench_comparison benchmarks/bench_comparison.cpp)
target_link_libraries(bench_comparison PRIVATE benchmark::benchmark)

//test_ring_buffer

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <numeric>
#include "spsc/ring_buffer.hpp"

TEST(SPSCRingBufferTest, SingleThreadBasicOps) {
    SPSCRingBuffer<int, 4> buffer;

    EXPECT_TRUE(buffer.push(10));
    EXPECT_TRUE(buffer.push(20));
    EXPECT_TRUE(buffer.push(30));
    EXPECT_TRUE(buffer.push(40));

    EXPECT_FALSE(buffer.push(50));

    int val1 = 0;
    ASSERT_TRUE(buffer.pop(val1));
    EXPECT_EQ(val1, 10);

    EXPECT_TRUE(buffer.push(50));

    int val2 = 0;
    ASSERT_TRUE(buffer.pop(val2));
    EXPECT_EQ(val2, 20);
}

TEST(SPSCRingBufferTest, EmptyAndFullStates) {
    SPSCRingBuffer<int, 2> buffer;

    int dummy = 0;
    EXPECT_FALSE(buffer.pop(dummy));

    EXPECT_TRUE(buffer.push(100));
    EXPECT_TRUE(buffer.push(200));
    EXPECT_FALSE(buffer.push(300));

    int val1 = 0;
    ASSERT_TRUE(buffer.pop(val1));
    EXPECT_EQ(val1, 100);

    int val2 = 0;
    ASSERT_TRUE(buffer.pop(val2));
    EXPECT_EQ(val2, 200);

    EXPECT_FALSE(buffer.pop(dummy));
}

TEST(SPSCRingBufferTest, ConcurrentSPSCStressTest) {
    constexpr size_t kCapacity = 1024;
    constexpr size_t kNumElements = 1000000;

    SPSCRingBuffer<size_t, kCapacity> buffer;
    std::vector<size_t> received_data;
    received_data.reserve(kNumElements);

    std::thread producer([&]() {
        for (size_t i = 0; i < kNumElements; ++i) {
            while (!(buffer.push(i))) std::this_thread::yield();
        }
    });

    std::thread consumer([&]() {
        size_t val = 0;
        for (size_t i = 0; i < kNumElements; ++i) {
            while (!buffer.pop(val)) std::this_thread::yield();
            received_data.push_back(val);
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(received_data.size(), kNumElements);
    for (size_t i = 0; i < kNumElements; ++i) EXPECT_EQ(received_data[i], i);
}

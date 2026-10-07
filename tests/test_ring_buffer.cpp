//test_ring

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

    EXPECT_FALSE(buffer.push(40));

    auto val1 = buffer.pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(*val1, 10);

    EXPECT_TRUE(buffer.push(40));

    auto val2 = buffer.pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(*val2, 20);
}

TEST(SPSCRingBufferTest, EmptyAndFullStates) {
    SPSCRingBuffer<int, 2> buffer;

    EXPECT_FALSE(buffer.pop().has_value());

    EXPECT_TRUE(buffer.push(100));
    EXPECT_FALSE(buffer.push(200));

    EXPECT_EQ(*buffer.pop(), 100);
    EXPECT_FALSE(buffer.pop().has_value());
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
        for (size_t i = 0; i < kNumElements; ++i) {
            std::optional<size_t> val;
            while (!(val = buffer.pop())) std::this_thread::yield();
            received_data.push_back(*val);
        }
    })

    producer.join();
    consumer.join();

    ASSERT_EQ(received_data.size(), kNumElements);
    for (size_t i = 0; i < kNumElements; ++i) EXPECT_EQ(received_data[i], i);
}

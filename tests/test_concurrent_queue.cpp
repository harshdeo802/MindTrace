#include <gtest/gtest.h>
#include "core/pipeline/concurrent_queue.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace mindtrace;

TEST(TestConcurrentQueue, PushAndTryPop) {
    ConcurrentQueue<int> queue;

    queue.push(42);
    queue.push(99);

    auto val1 = queue.try_pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(*val1, 42);

    auto val2 = queue.try_pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(*val2, 99);

    auto val3 = queue.try_pop();
    EXPECT_FALSE(val3.has_value());
}

TEST(TestConcurrentQueue, BlockingPop) {
    ConcurrentQueue<int> queue;

    std::thread producer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.push(123);
    });

    auto val = queue.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 123);

    producer.join();
}

TEST(TestConcurrentQueue, TimedPop) {
    ConcurrentQueue<int> queue;

    auto val = queue.pop_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(val.has_value());  // Should timeout

    queue.push(77);
    auto val2 = queue.pop_for(std::chrono::milliseconds(100));
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(*val2, 77);
}

TEST(TestConcurrentQueue, StopUnblocksWaiters) {
    ConcurrentQueue<int> queue;

    std::atomic<bool> popped{false};
    std::thread consumer([&] {
        auto val = queue.pop();
        popped = true;
        EXPECT_FALSE(val.has_value());  // Should return nullopt on stop
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.stop();

    consumer.join();
    EXPECT_TRUE(popped);
}

TEST(TestConcurrentQueue, SizeAndEmpty) {
    ConcurrentQueue<std::string> queue;

    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0u);

    queue.push("hello");
    queue.push("world");

    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 2u);
}

TEST(TestConcurrentQueue, MultiProducerSingleConsumer) {
    ConcurrentQueue<int> queue;
    const int num_producers = 4;
    const int items_per_producer = 100;

    std::vector<std::thread> producers;
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([&queue, p, items_per_producer] {
            for (int i = 0; i < items_per_producer; ++i) {
                queue.push(p * 1000 + i);
            }
        });
    }

    for (auto& t : producers) t.join();

    EXPECT_EQ(queue.size(), static_cast<size_t>(num_producers * items_per_producer));

    // Consume all
    int count = 0;
    while (auto val = queue.try_pop()) {
        ++count;
    }
    EXPECT_EQ(count, num_producers * items_per_producer);
}

TEST(TestConcurrentQueue, FIFOOrdering) {
    ConcurrentQueue<int> queue;

    for (int i = 0; i < 10; ++i) {
        queue.push(i);
    }

    for (int i = 0; i < 10; ++i) {
        auto val = queue.try_pop();
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, i);
    }
}

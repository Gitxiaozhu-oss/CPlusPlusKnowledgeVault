#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include "hcstl/concurrent_vector.hpp"
#include "hcstl/concurrent_queue.hpp"
#include "hcstl/concurrent_map.hpp"

using namespace hcstl;

// 并发向量测试
TEST(ConcurrentVectorTest, BasicOperations) 
{
    concurrent_vector<int> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0);

    vec.push_back(1);
    EXPECT_FALSE(vec.empty());
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec.at(0), 1);
}

TEST(ConcurrentVectorTest, ConcurrentPushBack) 
{
    concurrent_vector<int> vec;
    const int num_threads = 4;
    const int num_iterations = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) 
    {
        threads.emplace_back([&vec, i]() 
        {
            for (int j = 0; j < 1000; ++j) 
            {
                vec.push_back(i * 1000 + j);
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    EXPECT_EQ(vec.size(), num_threads * num_iterations);
}

// 并发队列测试
TEST(ConcurrentQueueTest, BasicOperations) 
{
    concurrent_queue<int> queue;
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(1);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    int value;
    EXPECT_TRUE(queue.try_pop(value));
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(queue.empty());
}

TEST(ConcurrentQueueTest, ProducerConsumer) 
{
    concurrent_queue<int> queue;
    const int num_producers = 4;
    const int num_consumers = 4;
    const int num_iterations = 1000;
    std::atomic<int> total_consumed{0};
    std::vector<std::thread> threads;

    // 生产者线程
    for (int i = 0; i < num_producers; ++i) 
    {
        threads.emplace_back([&queue, i]() 
        {
            for (int j = 0; j < 1000; ++j) 
            {
                queue.push(i * 1000 + j);
            }
        });
    }

    // 消费者线程
    for (int i = 0; i < num_consumers; ++i) 
    {
        threads.emplace_back([&queue, &total_consumed]() 
        {
            int value;
            while (total_consumed.load() < 4000) 
            {
                if (queue.try_pop(value)) 
                {
                    total_consumed.fetch_add(1);
                }
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    EXPECT_EQ(total_consumed.load(), num_producers * num_iterations);
    EXPECT_TRUE(queue.empty());
}

// 并发映射测试
TEST(ConcurrentMapTest, BasicOperations) 
{
    concurrent_map<int, std::string> map;
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.size(), 0);

    EXPECT_TRUE(map.insert(1, "one"));
    EXPECT_FALSE(map.empty());
    EXPECT_EQ(map.size(), 1);

    auto value = map.find(1);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(value.value(), "one");

    EXPECT_TRUE(map.erase(1));
    EXPECT_TRUE(map.empty());
    EXPECT_FALSE(map.find(1).has_value());
}

TEST(ConcurrentMapTest, ConcurrentOperations) 
{
    concurrent_map<int, int> map;
    const int num_threads = 4;
    const int num_iterations = 1000;
    std::vector<std::thread> threads;

    // 并发插入
    for (int i = 0; i < num_threads; ++i) 
    {
        threads.emplace_back([&map, i]() 
        {
            for (int j = 0; j < 1000; ++j) 
            {
                int key = i * 1000 + j;
                map.insert(key, key * 2);
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    EXPECT_EQ(map.size(), num_threads * num_iterations);

    // 验证所有插入的值
    for (int i = 0; i < num_threads; ++i) 
    {
        for (int j = 0; j < num_iterations; ++j) 
        {
            int key = i * num_iterations + j;
            auto value = map.find(key);
            EXPECT_TRUE(value.has_value());
            EXPECT_EQ(value.value(), key * 2);
        }
    }
} 
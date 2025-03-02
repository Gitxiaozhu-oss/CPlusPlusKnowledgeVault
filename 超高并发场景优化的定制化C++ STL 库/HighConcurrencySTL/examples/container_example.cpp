#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include "hcstl/concurrent_vector.hpp"
#include "hcstl/concurrent_queue.hpp"
#include "hcstl/concurrent_map.hpp"

using namespace hcstl;
using namespace std::chrono;

void test_concurrent_vector() 
{
    std::cout << "\n=== 测试并发向量 ===\n";
    concurrent_vector<int> vec;
    const int num_threads = 4;
    std::vector<std::thread> threads;

    auto start = high_resolution_clock::now();

    // 创建多个线程并发插入元素
    for (int i = 0; i < num_threads; ++i) 
    {
        threads.emplace_back([&vec, i]() 
        {
            for (int j = 0; j < 10000; ++j) 
            {
                vec.push_back(i * 10000 + j);
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "插入 " << vec.size() << " 个元素用时: " 
              << duration.count() << " 毫秒\n";
}

void test_concurrent_queue() 
{
    std::cout << "\n=== 测试并发队列 ===\n";
    concurrent_queue<int> queue;
    const int num_producers = 2;
    const int num_consumers = 2;
    std::atomic<int> total_consumed{0};
    std::vector<std::thread> threads;
    std::atomic<bool> stop{false};

    auto start = high_resolution_clock::now();

    // 创建生产者线程
    for (int i = 0; i < num_producers; ++i) 
    {
        threads.emplace_back([&queue, i]() 
        {
            for (int j = 0; j < 10000; ++j) 
            {
                queue.push(i * 10000 + j);
            }
        });
    }

    // 创建消费者线程
    for (int i = 0; i < num_consumers; ++i) 
    {
        threads.emplace_back([&queue, &total_consumed, &stop]() 
        {
            int value;
            while (!stop.load() || !queue.empty()) 
            {
                if (queue.try_pop(value)) 
                {
                    total_consumed.fetch_add(1);
                }
            }
        });
    }

    // 等待生产者完成
    for (int i = 0; i < num_producers; ++i) 
    {
        threads[i].join();
    }
    stop.store(true);

    // 等待消费者完成
    for (int i = num_producers; i < num_producers + num_consumers; ++i) 
    {
        threads[i].join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "处理 " << total_consumed.load() << " 个元素用时: " 
              << duration.count() << " 毫秒\n";
}

void test_concurrent_map() 
{
    std::cout << "\n=== 测试并发映射 ===\n";
    concurrent_map<int, std::string> map;
    const int num_threads = 4;
    std::vector<std::thread> threads;

    auto start = high_resolution_clock::now();

    // 创建多个线程并发插入和查找
    for (int i = 0; i < num_threads; ++i) 
    {
        threads.emplace_back([&map, i]() 
        {
            for (int j = 0; j < 1000; ++j) 
            {
                int key = i * 1000 + j;
                map.insert(key, "value-" + std::to_string(key));
                auto value = map.find(key);
                if (!value.has_value()) 
                {
                    std::cout << "错误：未找到键 " << key << "\n";
                }
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    std::cout << "处理 " << map.size() << " 个元素用时: " 
              << duration.count() << " 毫秒\n";
}

int main() 
{
    std::cout << "开始测试高性能并发容器...\n";

    test_concurrent_vector();
    test_concurrent_queue();
    test_concurrent_map();

    std::cout << "\n所有测试完成！\n";
    return 0;
} 
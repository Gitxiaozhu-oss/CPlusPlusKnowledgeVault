#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <queue>
#include <map>
#include "hcstl/concurrent_vector.hpp"
#include "hcstl/concurrent_queue.hpp"
#include "hcstl/concurrent_map.hpp"

using namespace hcstl;

// 并发向量基准测试
static void BM_ConcurrentVectorPushBack(benchmark::State& state) 
{
    const int num_threads = state.range(0);

    for (auto _ : state) 
    {
        concurrent_vector<int> vec;
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) 
        {
            threads.emplace_back([&vec]() 
            {
                for (int j = 0; j < 100; ++j) 
                {
                    vec.push_back(j);
                }
            });
        }

        for (auto& thread : threads) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(vec.size());
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 100);
}
BENCHMARK(BM_ConcurrentVectorPushBack)->Range(1, 4)->UseRealTime();

// 标准向量基准测试（带锁）
static void BM_StdVectorPushBack(benchmark::State& state) 
{
    const int num_threads = state.range(0);

    for (auto _ : state) 
    {
        std::vector<int> vec;
        std::mutex mutex;
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) 
        {
            threads.emplace_back([&vec, &mutex]() 
            {
                for (int j = 0; j < 100; ++j) 
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    vec.push_back(j);
                }
            });
        }

        for (auto& thread : threads) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(vec.size());
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 100);
}
BENCHMARK(BM_StdVectorPushBack)->Range(1, 4)->UseRealTime();

// 并发队列基准测试
static void BM_ConcurrentQueuePushPop(benchmark::State& state) 
{
    const int num_threads = state.range(0);
    const int items_per_thread = 100;
    const size_t total_items = (num_threads / 2) * items_per_thread;

    for (auto _ : state) 
    {
        concurrent_queue<int> queue;
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        std::atomic<size_t> total_produced{0};
        std::atomic<size_t> total_consumed{0};

        producers.reserve(num_threads / 2);
        consumers.reserve(num_threads / 2);

        // 生产者线程
        for (int i = 0; i < num_threads / 2; ++i) 
        {
            producers.emplace_back([&queue, &total_produced]() 
            {
                for (int j = 0; j < 100; ++j) 
                {
                    queue.push(j);
                    total_produced.fetch_add(1, std::memory_order_release);
                }
            });
        }

        // 消费者线程
        for (int i = 0; i < num_threads / 2; ++i) 
        {
            consumers.emplace_back([&queue, &total_consumed, total_items]() 
            {
                int value;
                while (total_consumed.load(std::memory_order_acquire) < total_items) 
                {
                    if (queue.try_pop(value)) 
                    {
                        total_consumed.fetch_add(1, std::memory_order_release);
                    }
                    std::this_thread::yield();
                }
            });
        }

        for (auto& thread : producers) 
        {
            thread.join();
        }

        for (auto& thread : consumers) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(total_consumed.load());
    }
    state.SetItemsProcessed(state.iterations() * (num_threads / 2) * 100);
}
BENCHMARK(BM_ConcurrentQueuePushPop)->Range(2, 4)->UseRealTime();

// 标准队列基准测试（带锁）
static void BM_StdQueuePushPop(benchmark::State& state) 
{
    const int num_threads = state.range(0);
    const int items_per_thread = 100;
    const size_t total_items = (num_threads / 2) * items_per_thread;

    for (auto _ : state) 
    {
        std::queue<int> queue;
        std::mutex mutex;
        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;
        std::atomic<size_t> total_produced{0};
        std::atomic<size_t> total_consumed{0};

        producers.reserve(num_threads / 2);
        consumers.reserve(num_threads / 2);

        // 生产者线程
        for (int i = 0; i < num_threads / 2; ++i) 
        {
            producers.emplace_back([&queue, &mutex, &total_produced]() 
            {
                for (int j = 0; j < 100; ++j) 
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    queue.push(j);
                    total_produced.fetch_add(1, std::memory_order_release);
                }
            });
        }

        // 消费者线程
        for (int i = 0; i < num_threads / 2; ++i) 
        {
            consumers.emplace_back([&queue, &mutex, &total_consumed, total_items]() 
            {
                while (total_consumed.load(std::memory_order_acquire) < total_items) 
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    if (!queue.empty()) 
                    {
                        queue.pop();
                        total_consumed.fetch_add(1, std::memory_order_release);
                    }
                }
            });
        }

        for (auto& thread : producers) 
        {
            thread.join();
        }

        for (auto& thread : consumers) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(total_consumed.load());
    }
    state.SetItemsProcessed(state.iterations() * (num_threads / 2) * 100);
}
BENCHMARK(BM_StdQueuePushPop)->Range(2, 4)->UseRealTime();

// 并发映射基准测试
static void BM_ConcurrentMapInsertFind(benchmark::State& state) 
{
    const int num_threads = state.range(0);

    for (auto _ : state) 
    {
        concurrent_map<int, int> map;
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) 
        {
            threads.emplace_back([&map, i]() 
            {
                for (int j = 0; j < 50; ++j) 
                {
                    int key = i * 50 + j;
                    map.insert(key, key * 2);
                    auto value = map.find(key);
                    benchmark::DoNotOptimize(value);
                }
            });
        }

        for (auto& thread : threads) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(map.size());
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 100); // 50 inserts + 50 finds
}
BENCHMARK(BM_ConcurrentMapInsertFind)->Range(1, 4)->UseRealTime();

// 标准映射基准测试（带锁）
static void BM_StdMapInsertFind(benchmark::State& state) 
{
    const int num_threads = state.range(0);

    for (auto _ : state) 
    {
        std::map<int, int> map;
        std::mutex mutex;
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        for (int i = 0; i < num_threads; ++i) 
        {
            threads.emplace_back([&map, &mutex, i]() 
            {
                for (int j = 0; j < 50; ++j) 
                {
                    int key = i * 50 + j;
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        map[key] = key * 2;
                    }
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        benchmark::DoNotOptimize(map.find(key));
                    }
                }
            });
        }

        for (auto& thread : threads) 
        {
            thread.join();
        }

        benchmark::DoNotOptimize(map.size());
    }
    state.SetItemsProcessed(state.iterations() * num_threads * 100); // 50 inserts + 50 finds
}
BENCHMARK(BM_StdMapInsertFind)->Range(1, 4)->UseRealTime(); 
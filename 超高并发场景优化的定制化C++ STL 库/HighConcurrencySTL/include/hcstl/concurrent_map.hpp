#ifndef HCSTL_CONCURRENT_MAP_HPP
#define HCSTL_CONCURRENT_MAP_HPP

#include <atomic>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

namespace hcstl {

/**
 * @brief 高性能并发哈希映射
 * 
 * @tparam Key 键类型
 * @tparam T 值类型
 * @tparam Hash 哈希函数类型
 * @tparam KeyEqual 键比较函数类型
 * @tparam Allocator 分配器类型
 */
template<
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>
>
class concurrent_map 
{
private:
    static constexpr size_t NUM_SHARDS = 32;
    static constexpr float MAX_LOAD_FACTOR = 0.75f;

    struct Node 
    {
        std::pair<const Key, T> data;
        std::atomic<Node*> next{nullptr};

        template<typename... Args>
        Node(Args&&... args) : data(std::forward<Args>(args)...) {}
    };

    struct Bucket 
    {
        std::atomic<Node*> head{nullptr};
        std::atomic<size_t> size{0};
        mutable std::shared_mutex mutex;
    };

    std::vector<Bucket> buckets_;
    std::atomic<size_t> size_{0};
    Hash hasher_;
    KeyEqual key_equal_;

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    NodeAllocator node_allocator_;

public:
    using key_type = Key;
    using mapped_type = T;
    using value_type = std::pair<const Key, T>;
    using size_type = std::size_t;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;

    /**
     * @brief 构造函数
     */
    concurrent_map() : buckets_(NUM_SHARDS) {}

    /**
     * @brief 析构函数
     */
    ~concurrent_map() 
    {
        clear();
    }

    /**
     * @brief 插入键值对
     * 
     * @param key 键
     * @param value 值
     * @return bool 如果插入成功返回true，如果键已存在返回false
     */
    bool insert(const Key& key, const T& value) 
    {
        size_t bucket_index = bucket_idx(key);
        Bucket& bucket = buckets_[bucket_index];
        std::unique_lock lock(bucket.mutex);

        Node* current = bucket.head.load();
        while (current) 
        {
            if (key_equal_(current->data.first, key)) 
            {
                return false;
            }
            current = current->next.load();
        }

        Node* new_node = node_allocator_.allocate(1);
        std::allocator_traits<NodeAllocator>::construct(node_allocator_, new_node, key, value);

        new_node->next = bucket.head.load();
        bucket.head.store(new_node);
        bucket.size.fetch_add(1, std::memory_order_relaxed);
        size_.fetch_add(1, std::memory_order_relaxed);

        return true;
    }

    /**
     * @brief 查找键对应的值
     * 
     * @param key 要查找的键
     * @return std::optional<T> 如果找到则返回值，否则返回空
     */
    std::optional<T> find(const Key& key) const 
    {
        size_t bucket_index = bucket_idx(key);
        const Bucket& bucket = buckets_[bucket_index];
        std::shared_lock lock(bucket.mutex);

        Node* current = bucket.head.load();
        while (current) 
        {
            if (key_equal_(current->data.first, key)) 
            {
                return current->data.second;
            }
            current = current->next.load();
        }

        return std::nullopt;
    }

    /**
     * @brief 删除指定键的元素
     * 
     * @param key 要删除的键
     * @return bool 如果成功删除返回true，如果键不存在返回false
     */
    bool erase(const Key& key) 
    {
        size_t bucket_index = bucket_idx(key);
        Bucket& bucket = buckets_[bucket_index];
        std::unique_lock lock(bucket.mutex);

        Node* current = bucket.head.load();
        Node* prev = nullptr;

        while (current) 
        {
            if (key_equal_(current->data.first, key)) 
            {
                if (prev) 
                {
                    prev->next.store(current->next.load());
                }
                else 
                {
                    bucket.head.store(current->next.load());
                }

                std::allocator_traits<NodeAllocator>::destroy(node_allocator_, current);
                node_allocator_.deallocate(current, 1);
                bucket.size.fetch_sub(1, std::memory_order_relaxed);
                size_.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }

            prev = current;
            current = current->next.load();
        }

        return false;
    }

    /**
     * @brief 获取映射的当前大小
     * 
     * @return size_type 映射中的元素数量
     */
    size_type size() const noexcept 
    {
        return size_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 检查映射是否为空
     * 
     * @return bool 如果映射为空返回true，否则返回false
     */
    bool empty() const noexcept 
    {
        return size() == 0;
    }

    /**
     * @brief 清空映射中的所有元素
     */
    void clear() 
    {
        for (Bucket& bucket : buckets_) 
        {
            std::unique_lock lock(bucket.mutex);
            Node* current = bucket.head.load();
            while (current) 
            {
                Node* next = current->next.load();
                std::allocator_traits<NodeAllocator>::destroy(node_allocator_, current);
                node_allocator_.deallocate(current, 1);
                current = next;
            }
            bucket.head.store(nullptr);
            bucket.size.store(0, std::memory_order_relaxed);
        }
        size_.store(0, std::memory_order_relaxed);
    }

private:
    /**
     * @brief 计算键的桶索引
     * 
     * @param key 键
     * @return size_t 桶索引
     */
    size_t bucket_idx(const Key& key) const 
    {
        return hasher_(key) % NUM_SHARDS;
    }
};

} // namespace hcstl

#endif // HCSTL_CONCURRENT_MAP_HPP 
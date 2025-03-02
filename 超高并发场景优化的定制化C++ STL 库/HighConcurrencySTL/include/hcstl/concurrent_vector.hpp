#ifndef HCSTL_CONCURRENT_VECTOR_HPP
#define HCSTL_CONCURRENT_VECTOR_HPP

#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include <shared_mutex>

namespace hcstl {

/**
 * @brief 高性能并发向量容器
 * 
 * @tparam T 元素类型
 * @tparam Allocator 分配器类型
 */
template<typename T, typename Allocator = std::allocator<T>>
class concurrent_vector 
{
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    static constexpr size_type SEGMENT_SIZE = 1024;

private:
    struct Segment 
    {
        std::array<T, SEGMENT_SIZE> data;
        std::atomic<size_type> size{0};
    };

    std::vector<std::unique_ptr<Segment>> segments_;
    std::atomic<size_type> size_{0};
    mutable std::shared_mutex mutex_;
    Allocator allocator_;

public:
    /**
     * @brief 构造函数
     */
    concurrent_vector() = default;

    /**
     * @brief 析构函数
     */
    ~concurrent_vector() = default;

    /**
     * @brief 向向量尾部添加元素
     * 
     * @param value 要添加的元素值
     */
    void push_back(const T& value) 
    {
        size_type current_size = size_.load(std::memory_order_relaxed);
        size_type segment_index = current_size / SEGMENT_SIZE;
        size_type offset = current_size % SEGMENT_SIZE;

        // 确保有足够的段
        ensure_segment(segment_index);

        // 添加元素
        segments_[segment_index]->data[offset] = value;
        segments_[segment_index]->size.fetch_add(1, std::memory_order_release);
        size_.fetch_add(1, std::memory_order_release);
    }

    /**
     * @brief 访问指定位置的元素
     * 
     * @param index 要访问的元素索引
     * @return const_reference 元素的常量引用
     * @throw std::out_of_range 如果索引超出范围
     */
    const_reference at(size_type index) const 
    {
        if (index >= size()) 
        {
            throw std::out_of_range("Index out of range");
        }

        size_type segment_index = index / SEGMENT_SIZE;
        size_type offset = index % SEGMENT_SIZE;

        std::shared_lock lock(mutex_);
        return segments_[segment_index]->data[offset];
    }

    /**
     * @brief 获取向量的当前大小
     * 
     * @return size_type 向量中的元素数量
     */
    size_type size() const noexcept 
    {
        return size_.load(std::memory_order_acquire);
    }

    /**
     * @brief 检查向量是否为空
     * 
     * @return bool 如果向量为空返回true，否则返回false
     */
    bool empty() const noexcept 
    {
        return size() == 0;
    }

private:
    /**
     * @brief 确保指定索引的段存在
     * 
     * @param segment_index 段索引
     */
    void ensure_segment(size_type segment_index) 
    {
        std::unique_lock lock(mutex_);
        if (segment_index >= segments_.size()) 
        {
            segments_.resize(segment_index + 1);
        }
        if (!segments_[segment_index]) 
        {
            segments_[segment_index] = std::make_unique<Segment>();
        }
    }
};

} // namespace hcstl

#endif // HCSTL_CONCURRENT_VECTOR_HPP 
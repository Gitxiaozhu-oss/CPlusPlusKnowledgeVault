#pragma once

#include <memory>
#include <span>
#include <vector>
#include <stdexcept>
#include <memory_resource>

namespace hsmm {
namespace utils {

/**
 * @brief 安全的缓冲区类
 * 
 * 提供边界检查和自动内存管理的缓冲区实现
 * @tparam T 缓冲区元素类型
 */
template<typename T>
class SafeBuffer {
public:
    /**
     * @brief 构造函数
     * @param size 缓冲区大小
     * @param resource 内存分配器
     */
    explicit SafeBuffer(size_t size, 
                       std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : data_(std::pmr::polymorphic_allocator<T>(resource)) {
        data_.resize(size);
    }

    /**
     * @brief 获取只读视图
     * @return 缓冲区的只读span
     */
    std::span<const T> view() const noexcept {
        return std::span<const T>(data_);
    }

    /**
     * @brief 获取可写视图
     * @return 缓冲区的可写span
     */
    std::span<T> view() noexcept {
        return std::span<T>(data_);
    }

    /**
     * @brief 安全的写入操作
     * @param pos 写入位置
     * @param value 要写入的值
     * @throws std::out_of_range 如果位置越界
     */
    void write(size_t pos, const T& value) {
        if (pos >= data_.size()) {
            throw std::out_of_range("Buffer write position out of range");
        }
        data_[pos] = value;
    }

    /**
     * @brief 安全的读取操作
     * @param pos 读取位置
     * @return 读取的值
     * @throws std::out_of_range 如果位置越界
     */
    const T& read(size_t pos) const {
        if (pos >= data_.size()) {
            throw std::out_of_range("Buffer read position out of range");
        }
        return data_[pos];
    }

    /**
     * @brief 获取缓冲区大小
     * @return 缓冲区大小
     */
    size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief 清空缓冲区
     */
    void clear() noexcept {
        data_.clear();
    }

    /**
     * @brief 重置缓冲区大小
     * @param new_size 新的缓冲区大小
     */
    void resize(size_t new_size) {
        data_.resize(new_size);
    }

private:
    std::pmr::vector<T> data_;
};

} // namespace utils
} // namespace hsmm 
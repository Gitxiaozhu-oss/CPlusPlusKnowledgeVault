#ifndef HCSTL_CONCURRENT_QUEUE_HPP
#define HCSTL_CONCURRENT_QUEUE_HPP

#include <atomic>
#include <memory>
#include <optional>

namespace hcstl {

/**
 * @brief 无锁并发队列
 * 
 * @tparam T 元素类型
 * @tparam Allocator 分配器类型
 */
template<typename T, typename Allocator = std::allocator<T>>
class concurrent_queue 
{
private:
    struct Node 
    {
        std::atomic<Node*> next{nullptr};
        T data;

        template<typename... Args>
        Node(Args&&... args) : data(std::forward<Args>(args)...) {}
    };

    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    std::atomic<size_t> size_{0};

    using NodeAllocator = typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    NodeAllocator node_allocator_;

public:
    using value_type = T;
    using size_type = std::size_t;
    using allocator_type = Allocator;

    /**
     * @brief 构造函数
     */
    concurrent_queue() 
    {
        Node* dummy = node_allocator_.allocate(1);
        std::allocator_traits<NodeAllocator>::construct(node_allocator_, dummy);
        head_.store(dummy);
        tail_.store(dummy);
    }

    /**
     * @brief 析构函数
     */
    ~concurrent_queue() 
    {
        Node* current = head_.load();
        while (current) 
        {
            Node* next = current->next.load();
            std::allocator_traits<NodeAllocator>::destroy(node_allocator_, current);
            node_allocator_.deallocate(current, 1);
            current = next;
        }
    }

    /**
     * @brief 将元素推入队列
     * 
     * @param value 要推入的元素值
     */
    void push(const T& value) 
    {
        Node* new_node = node_allocator_.allocate(1);
        std::allocator_traits<NodeAllocator>::construct(node_allocator_, new_node, value);

        while (true) 
        {
            Node* tail = tail_.load();
            Node* next = tail->next.load();

            if (tail == tail_.load()) 
            {
                if (next == nullptr) 
                {
                    if (tail->next.compare_exchange_weak(next, new_node)) 
                    {
                        tail_.compare_exchange_strong(tail, new_node);
                        size_.fetch_add(1, std::memory_order_relaxed);
                        return;
                    }
                }
                else 
                {
                    tail_.compare_exchange_strong(tail, next);
                }
            }
        }
    }

    /**
     * @brief 尝试从队列中弹出元素
     * 
     * @param value 用于存储弹出元素的引用
     * @return bool 如果成功弹出返回true，队列为空返回false
     */
    bool try_pop(T& value) 
    {
        while (true) 
        {
            Node* head = head_.load();
            Node* tail = tail_.load();
            Node* next = head->next.load();

            if (head == head_.load()) 
            {
                if (head == tail) 
                {
                    if (next == nullptr) 
                    {
                        return false;
                    }
                    tail_.compare_exchange_strong(tail, next);
                }
                else 
                {
                    value = next->data;
                    if (head_.compare_exchange_weak(head, next)) 
                    {
                        std::allocator_traits<NodeAllocator>::destroy(node_allocator_, head);
                        node_allocator_.deallocate(head, 1);
                        size_.fetch_sub(1, std::memory_order_relaxed);
                        return true;
                    }
                }
            }
        }
    }

    /**
     * @brief 获取队列的当前大小
     * 
     * @return size_type 队列中的元素数量
     */
    size_type size() const noexcept 
    {
        return size_.load(std::memory_order_relaxed);
    }

    /**
     * @brief 检查队列是否为空
     * 
     * @return bool 如果队列为空返回true，否则返回false
     */
    bool empty() const noexcept 
    {
        return size() == 0;
    }
};

} // namespace hcstl

#endif // HCSTL_CONCURRENT_QUEUE_HPP 
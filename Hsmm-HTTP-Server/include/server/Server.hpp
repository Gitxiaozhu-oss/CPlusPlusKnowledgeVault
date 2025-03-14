#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <memory_resource>
#include <atomic>

namespace hsmm {

/**
 * @brief HTTP服务器核心类
 * 
 * 使用RAII管理资源生命周期，实现内存安全的HTTP服务器
 */
class Server {
public:
    /**
     * @brief 构造函数
     * @param address 服务器监听地址
     * @param port 服务器监听端口
     * @param thread_count 工作线程数量
     */
    Server(const std::string& address, unsigned short port, size_t thread_count = std::thread::hardware_concurrency());

    /**
     * @brief 启动服务器
     * @throws boost::system::system_error 如果启动失败
     */
    void run();

    /**
     * @brief 停止服务器
     */
    void stop();

private:
    /**
     * @brief 接受新的连接
     */
    void do_accept();

    /**
     * @brief 工作线程函数
     */
    void worker_thread();

private:
    boost::asio::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::vector<std::thread> worker_threads_;
    
    // 使用PMR内存池管理连接对象
    std::pmr::synchronized_pool_resource connection_pool_;
    
    // 服务器配置
    const size_t thread_count_;
    std::atomic<bool> running_{false};
};

} // namespace hsmm 
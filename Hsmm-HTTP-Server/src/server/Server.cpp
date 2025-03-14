#include "server/Server.hpp"
#include "server/Connection.hpp"
#include "utils/Logger.hpp"
#include <boost/asio/signal_set.hpp>
#include <memory>
#include <stdexcept>

namespace hsmm {

Server::Server(const std::string& address, unsigned short port, size_t thread_count)
    : io_context_(thread_count)  // 设置并发线程数
    , work_guard_(boost::asio::make_work_guard(io_context_))  // 防止io_context过早退出
    , acceptor_(io_context_)
    , thread_count_(thread_count) {
    
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::make_address(address), port);
    
    // 配置接受器
    boost::system::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        LOG_ERROR("打开接受器失败: " + ec.message());
        throw std::runtime_error("打开接受器失败: " + ec.message());
    }

    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec) {
        LOG_ERROR("设置地址重用选项失败: " + ec.message());
        throw std::runtime_error("设置地址重用选项失败: " + ec.message());
    }

    acceptor_.bind(endpoint, ec);
    if (ec) {
        LOG_ERROR("绑定地址失败: " + ec.message());
        throw std::runtime_error("绑定地址失败: " + ec.message());
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        LOG_ERROR("开始监听失败: " + ec.message());
        throw std::runtime_error("开始监听失败: " + ec.message());
    }

    LOG_INFO("服务器初始化完成，监听地址: " + address + ":" + std::to_string(port));
}

void Server::run() {
    running_ = true;

    // 启动工作线程
    LOG_INFO("启动工作线程池，线程数: " + std::to_string(thread_count_));
    for (size_t i = 0; i < thread_count_ - 1; ++i) {
        worker_threads_.emplace_back([this] { worker_thread(); });
    }

    // 启动接受连接
    do_accept();

    // 主线程也运行io_context
    try {
        io_context_.run();
        LOG_INFO("服务器主循环退出");
    }
    catch (const std::exception& e) {
        LOG_ERROR("主线程异常: " + std::string(e.what()));
        stop();
    }
}

void Server::stop() {
    if (!running_) return;

    LOG_INFO("正在停止服务器...");
    running_ = false;
    
    // 停止接受新的工作
    work_guard_.reset();
    
    // 停止io_context
    io_context_.stop();

    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    LOG_INFO("服务器已停止");
}

void Server::do_accept() {
    if (!running_) return;

    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                try {
                    LOG_DEBUG("接受新连接: " + socket.remote_endpoint().address().to_string() + 
                             ":" + std::to_string(socket.remote_endpoint().port()));

                    // 使用PMR内存池创建连接对象
                    auto connection = std::allocate_shared<Connection>(
                        std::pmr::polymorphic_allocator<Connection>(&connection_pool_),
                        std::move(socket));
                    
                    connection->start();
                }
                catch (const std::exception& e) {
                    LOG_ERROR("处理新连接时发生错误: " + std::string(e.what()));
                }
            } else {
                LOG_ERROR("接受连接失败: " + ec.message());
            }

            // 继续接受下一个连接
            if (running_) {
                do_accept();
            }
        });
}

void Server::worker_thread() {
    try {
        io_context_.run();
        LOG_INFO("工作线程退出");
    }
    catch (const std::exception& e) {
        LOG_ERROR("工作线程异常: " + std::string(e.what()));
    }
}
} 
#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string_view>
#include <span>
#include <array>

namespace hsmm {

/**
 * @brief HTTP连接类
 * 
 * 使用RAII管理单个HTTP连接的生命周期
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
    /**
     * @brief 构造函数
     * @param socket 已接受的TCP socket
     */
    explicit Connection(boost::asio::ip::tcp::socket socket);

    /**
     * @brief 开始处理连接
     */
    void start();

private:
    /**
     * @brief 异步读取HTTP请求
     */
    void do_read();

    /**
     * @brief 异步发送HTTP响应
     * @param response 要发送的响应数据
     */
    void do_write(std::string_view response);

    /**
     * @brief 解析HTTP请求
     * @param data 接收到的数据
     * @param length 数据长度
     * @return 是否解析成功
     */
    bool parse_request(std::span<const char> data);

private:
    boost::asio::ip::tcp::socket socket_;
    
    // 使用固定大小的缓冲区，防止缓冲区溢出
    static constexpr size_t max_buffer_size = 8192;
    std::array<char, max_buffer_size> buffer_;
    
    // 请求解析结果
    std::string_view method_;
    std::string_view uri_;
    std::string_view version_;
};

} // namespace hsmm 
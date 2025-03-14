#include "server/Connection.hpp"
#include "http/HttpParser.hpp"
#include "utils/Logger.hpp"
#include <boost/asio.hpp>
#include <memory>
#include <string>

namespace hsmm {

Connection::Connection(boost::asio::ip::tcp::socket socket)
    : socket_(std::move(socket)) {
}

void Connection::start() {
    do_read();
}

void Connection::do_read() {
    auto self(shared_from_this());
    
    socket_.async_read_some(
        boost::asio::buffer(buffer_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                // 解析HTTP请求
                if (auto request = http::HttpParser::parse(std::span<const char>(buffer_.data(), length))) {
                    // 创建响应
                    http::HttpResponse response;
                    response.set_status(200, "OK");
                    response.add_header("Content-Type", "text/plain");
                    response.add_header("Server", "HSMM-HTTP-Server");
                    response.set_body("Hello from HSMM HTTP Server!");

                    // 发送响应
                    do_write(response.to_string());
                } else {
                    // 请求解析失败
                    http::HttpResponse response;
                    response.set_status(400, "Bad Request");
                    response.add_header("Content-Type", "text/plain");
                    response.set_body("Invalid HTTP request");
                    
                    do_write(response.to_string());
                }
            } else if (ec != boost::asio::error::operation_aborted) {
                LOG_ERROR("读取连接数据失败: " + std::string(ec.message()));
            }
        });
}

void Connection::do_write(std::string_view response) {
    auto self(shared_from_this());
    
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(response.data(), response.size()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                // 继续读取下一个请求
                do_read();
            } else if (ec != boost::asio::error::operation_aborted) {
                LOG_ERROR("写入响应数据失败: " + std::string(ec.message()));
            }
        });
}

bool Connection::parse_request(std::span<const char> data) {
    // 使用HttpParser解析请求
    auto request = http::HttpParser::parse(data);
    if (!request) {
        return false;
    }

    method_ = request->method;
    uri_ = request->uri;
    version_ = request->version;

    return true;
}
} 
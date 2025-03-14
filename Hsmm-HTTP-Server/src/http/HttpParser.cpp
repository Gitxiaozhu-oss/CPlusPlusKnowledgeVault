#include "http/HttpParser.hpp"
#include "utils/Logger.hpp"
#include <algorithm>
#include <string_view>
#include <vector>

namespace hsmm {
namespace http {

namespace {
    // 辅助函数：去除字符串两端的空白字符
    std::string_view trim(std::string_view str) {
        const auto start = str.find_first_not_of(" \t\r\n");
        if (start == std::string_view::npos) return {};
        
        const auto end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // 辅助函数：分割字符串
    std::vector<std::string_view> split(std::string_view str, char delim) {
        std::vector<std::string_view> result;
        size_t start = 0;
        size_t end = str.find(delim);

        while (end != std::string_view::npos) {
            result.push_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delim, start);
        }

        if (start < str.length()) {
            result.push_back(str.substr(start));
        }

        return result;
    }
}

std::optional<HttpRequest> HttpParser::parse(std::span<const char> data) {
    // 将数据转换为string_view
    std::string_view content(data.data(), data.size());
    
    // 查找请求头结束位置
    const auto header_end = content.find("\r\n\r\n");
    if (header_end == std::string_view::npos) {
        LOG_WARNING("无法找到HTTP请求头结束标记");
        return std::nullopt;
    }

    // 分割请求头和请求体
    auto headers = content.substr(0, header_end);
    auto body = content.substr(header_end + 4);

    // 按行分割请求头
    auto lines = split(headers, '\n');
    if (lines.empty()) {
        LOG_WARNING("空的HTTP请求");
        return std::nullopt;
    }

    HttpRequest request;

    // 解析请求行
    if (!parse_request_line(trim(lines[0]), request)) {
        LOG_WARNING("解析请求行失败");
        return std::nullopt;
    }

    // 解析请求头
    for (size_t i = 1; i < lines.size(); ++i) {
        if (!parse_header(trim(lines[i]), request)) {
            LOG_WARNING("解析请求头失败: " + std::string(lines[i]));
            return std::nullopt;
        }
    }

    // 设置请求体
    request.body = body;

    return request;
}

bool HttpParser::parse_request_line(std::string_view line, HttpRequest& request) {
    auto parts = split(line, ' ');
    if (parts.size() != 3) {
        return false;
    }

    request.method = parts[0];
    request.uri = parts[1];
    request.version = parts[2];

    return true;
}

bool HttpParser::parse_header(std::string_view line, HttpRequest& request) {
    if (line.empty()) return true;

    const auto separator = line.find(':');
    if (separator == std::string_view::npos) {
        return false;
    }

    auto name = trim(line.substr(0, separator));
    auto value = trim(line.substr(separator + 1));

    request.headers.emplace(name, value);
    return true;
}

void HttpResponse::set_status(int code, std::string_view message) {
    status_code_ = code;
    status_message_ = message;
}

void HttpResponse::add_header(std::string_view name, std::string_view value) {
    headers_.emplace(name, value);
}

void HttpResponse::set_body(std::string_view body) {
    body_ = body;
}

std::string HttpResponse::to_string() const {
    std::stringstream ss;
    
    // 状态行
    ss << "HTTP/1.1 " << status_code_ << " " << status_message_ << "\r\n";
    
    // 添加Content-Length头
    ss << "Content-Length: " << body_.length() << "\r\n";
    
    // 其他头部字段
    for (const auto& [name, value] : headers_) {
        ss << name << ": " << value << "\r\n";
    }
    
    // 空行分隔头部和主体
    ss << "\r\n";
    
    // 响应主体
    ss << body_;
    
    return ss.str();
}

} // namespace http
} // namespace hsmm 
#pragma once

#include <string_view>
#include <span>
#include <optional>
#include <unordered_map>

namespace hsmm {
namespace http {

/**
 * @brief HTTP请求解析结果
 */
struct HttpRequest {
    std::string_view method;
    std::string_view uri;
    std::string_view version;
    std::unordered_map<std::string_view, std::string_view> headers;
    std::string_view body;
};

/**
 * @brief HTTP响应生成器
 */
class HttpResponse {
public:
    /**
     * @brief 设置状态码和状态消息
     * @param code HTTP状态码
     * @param message 状态消息
     */
    void set_status(int code, std::string_view message);

    /**
     * @brief 添加响应头
     * @param name 头部字段名
     * @param value 头部字段值
     */
    void add_header(std::string_view name, std::string_view value);

    /**
     * @brief 设置响应体
     * @param body 响应体内容
     */
    void set_body(std::string_view body);

    /**
     * @brief 生成完整的HTTP响应
     * @return 格式化后的HTTP响应字符串
     */
    std::string to_string() const;

private:
    int status_code_{200};
    std::string_view status_message_{"OK"};
    std::unordered_map<std::string_view, std::string_view> headers_;
    std::string_view body_;
};

/**
 * @brief HTTP请求解析器
 */
class HttpParser {
public:
    /**
     * @brief 解析HTTP请求
     * @param data 请求数据
     * @return 解析结果，如果解析失败返回std::nullopt
     */
    static std::optional<HttpRequest> parse(std::span<const char> data);

private:
    /**
     * @brief 解析请求行
     * @param line 请求行数据
     * @param request 解析结果存储对象
     * @return 是否解析成功
     */
    static bool parse_request_line(std::string_view line, HttpRequest& request);

    /**
     * @brief 解析头部字段
     * @param line 头部字段行
     * @param request 解析结果存储对象
     * @return 是否解析成功
     */
    static bool parse_header(std::string_view line, HttpRequest& request);
};

} // namespace http
} // namespace hsmm 
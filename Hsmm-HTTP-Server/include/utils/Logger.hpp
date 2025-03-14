#pragma once

#include <string>
#include <string_view>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace hsmm {
namespace utils {

/**
 * @brief 日志级别枚举
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

/**
 * @brief 线程安全的日志类
 */
class Logger {
public:
    /**
     * @brief 获取日志单例实例
     * @return Logger实例的引用
     */
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    /**
     * @brief 初始化日志系统
     * @param filename 日志文件名
     * @param level 最低日志级别
     */
    void init(const std::string& filename, LogLevel level = LogLevel::INFO) {
        std::lock_guard<std::mutex> lock(mutex_);
        file_.open(filename, std::ios::app);
        level_ = level;
    }

    /**
     * @brief 写入日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, std::string_view message) {
        if (level < level_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] "
           << level_to_string(level) << ": "
           << message << std::endl;

        if (file_.is_open()) {
            file_ << ss.str();
            file_.flush();
        }
        
        // 同时输出到控制台
        std::cout << ss.str();
    }

    // 便捷日志方法
    void debug(std::string_view message) { log(LogLevel::DEBUG, message); }
    void info(std::string_view message) { log(LogLevel::INFO, message); }
    void warning(std::string_view message) { log(LogLevel::WARNING, message); }
    void error(std::string_view message) { log(LogLevel::ERROR, message); }

private:
    Logger() = default;
    ~Logger() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    // 禁止拷贝和移动
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief 将日志级别转换为字符串
     * @param level 日志级别
     * @return 对应的字符串表示
     */
    static std::string_view level_to_string(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

private:
    std::mutex mutex_;
    std::ofstream file_;
    LogLevel level_{LogLevel::INFO};
};

} // namespace utils
} // namespace hsmm

// 便捷宏定义
#define LOG_DEBUG(msg) hsmm::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg) hsmm::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg) hsmm::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg) hsmm::utils::Logger::instance().error(msg) 
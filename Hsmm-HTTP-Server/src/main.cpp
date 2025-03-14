#include "server/Server.hpp"
#include "utils/Logger.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <csignal>
#include <filesystem>

namespace {
    std::unique_ptr<hsmm::Server> server;

    void signal_handler(int signal) {
        if (server) {
            std::cout << "\n正在关闭服务器..." << std::endl;
            server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        // 初始化日志系统
        auto log_dir = std::filesystem::current_path() / "logs";
        std::filesystem::create_directories(log_dir);
        hsmm::utils::Logger::instance().init((log_dir / "server.log").string());
        
        LOG_INFO("正在启动服务器...");

        // 设置信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 默认配置
        const char* address = "0.0.0.0";
        unsigned short port = 8080;
        size_t thread_count = std::thread::hardware_concurrency();

        // 解析命令行参数
        if (argc > 1) address = argv[1];
        if (argc > 2) port = static_cast<unsigned short>(std::atoi(argv[2]));
        if (argc > 3) thread_count = static_cast<size_t>(std::atoi(argv[3]));

        // 创建并启动服务器
        LOG_INFO("配置信息：");
        LOG_INFO("  - 监听地址: " + std::string(address));
        LOG_INFO("  - 端口: " + std::to_string(port));
        LOG_INFO("  - 线程数: " + std::to_string(thread_count));

        server = std::make_unique<hsmm::Server>(address, port, thread_count);
        
        LOG_INFO("服务器创建成功，开始运行...");
        server->run();

        return 0;
    }
    catch (const std::exception& e) {
        LOG_ERROR("服务器启动失败: " + std::string(e.what()));
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
} 
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <iomanip>
#include <mutex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

// 统计信息
struct Statistics {
    std::atomic<size_t> success_count{0};
    std::atomic<size_t> error_count{0};
    std::atomic<size_t> total_latency{0};  // 单位：毫秒
    std::mutex cout_mutex;

    void print_progress(size_t total_requests) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        size_t completed = success_count + error_count;
        double progress = (completed * 100.0) / total_requests;
        double avg_latency = completed > 0 ? total_latency.load() / (double)completed : 0;
        
        std::cout << "\r进度: " << std::fixed << std::setprecision(2) << progress << "% "
                  << "成功: " << success_count << " "
                  << "失败: " << error_count << " "
                  << "平均延迟: " << avg_latency << "ms" << std::flush;
    }

    void print_final_results(size_t total_requests, std::chrono::milliseconds duration) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        double total_time = duration.count() / 1000.0;
        double qps = total_requests / total_time;
        double avg_latency = (success_count + error_count) > 0 ? 
            total_latency.load() / (double)(success_count + error_count) : 0;

        std::cout << "\n\n最终测试结果:" << std::endl;
        std::cout << "总请求数: " << total_requests << std::endl;
        std::cout << "成功请求: " << success_count << std::endl;
        std::cout << "失败请求: " << error_count << std::endl;
        std::cout << "总耗时: " << total_time << "秒" << std::endl;
        std::cout << "QPS: " << std::fixed << std::setprecision(2) << qps << std::endl;
        std::cout << "平均延迟: " << avg_latency << "ms" << std::endl;
    }
};

// 发送单个请求
void send_request(net::io_context& ioc, const std::string& host, unsigned short port, Statistics& stats) {
    try {
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto start_time = std::chrono::steady_clock::now();

        // 设置超时
        stream.expires_after(std::chrono::seconds(5));

        // 解析地址
        auto const results = resolver.resolve(host, std::to_string(port));
        
        // 连接
        stream.connect(results);

        // 准备请求
        http::request<http::string_body> req{http::verb::get, "/", 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, "StressTest");

        // 发送请求
        http::write(stream, req);

        // 接收响应
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        // 计算延迟
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // 更新统计信息
        stats.total_latency += latency.count();
        stats.success_count++;

        // 关闭连接
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }
    catch(std::exception const&) {
        stats.error_count++;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 5) {
            std::cerr << "用法: " << argv[0] << " <host> <port> <total_requests> <concurrent_connections>\n";
            return 1;
        }

        auto const host = argv[1];
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
        auto const total_requests = std::atoi(argv[3]);
        auto const concurrent_connections = std::atoi(argv[4]);

        std::cout << "开始压力测试:\n"
                  << "目标服务器: " << host << ":" << port << "\n"
                  << "总请求数: " << total_requests << "\n"
                  << "并发连接数: " << concurrent_connections << "\n\n";

        Statistics stats;
        auto start_time = std::chrono::steady_clock::now();

        // 创建线程池
        std::vector<std::thread> threads;
        size_t requests_per_thread = total_requests / concurrent_connections;
        size_t remaining_requests = total_requests % concurrent_connections;

        // 启动工作线程
        for (int i = 0; i < concurrent_connections; ++i) {
            size_t thread_requests = requests_per_thread + (i < remaining_requests ? 1 : 0);
            threads.emplace_back([&host, port, thread_requests, &stats, total_requests]() {
                net::io_context ioc;
                for (size_t j = 0; j < thread_requests; ++j) {
                    send_request(ioc, host, port, stats);
                    if (j % 10 == 0) {  // 每10个请求更新一次进度
                        stats.print_progress(total_requests);
                    }
                }
            });
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        // 打印最终结果
        stats.print_final_results(total_requests, duration);

        return 0;
    }
    catch(std::exception const& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
} 
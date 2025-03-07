#pragma once

#include <armadillo>
#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace rf {

/**
 * @brief 性能评估和监控类
 * 
 * 实现了模型评估、性能监控和资源使用统计功能
 */
class Metrics {
public:
    /**
     * @brief 回归指标结构体
     */
    struct RegressionMetrics {
        double mse;                 ///< 均方误差
        double rmse;               ///< 均方根误差
        double mae;                ///< 平均绝对误差
        double r2;                 ///< R²分数
        double explained_variance; ///< 解释方差分数
    };
    
    /**
     * @brief 计时器结构体
     */
    struct Timer {
        std::chrono::high_resolution_clock::time_point start_time;
        double total_time = 0.0;
        size_t count = 0;
    };
    
    /**
     * @brief 内存使用结构体
     */
    struct MemoryUsage {
        size_t current_usage = 0;
        size_t peak_usage = 0;
        std::vector<size_t> history;
    };
    
    /**
     * @brief 训练进度结构体
     */
    struct TrainingProgress {
        size_t epoch;
        RegressionMetrics metrics;
        double time;
    };
    
    /**
     * @brief 构造函数
     */
    Metrics();
    
    /**
     * @brief 计算回归指标
     * @param y_true 真实值
     * @param y_pred 预测值
     * @return 回归指标
     */
    static RegressionMetrics calculate_regression_metrics(const arma::vec& y_true,
                                                        const arma::vec& y_pred);
    
    /**
     * @brief 开始计时
     * @param name 计时器名称
     */
    void start_timer(const std::string& name);
    
    /**
     * @brief 停止计时
     * @param name 计时器名称
     * @return 耗时（毫秒）
     */
    double stop_timer(const std::string& name);
    
    /**
     * @brief 记录内存使用
     * @param name 记录名称
     * @param bytes 使用字节数
     */
    void record_memory_usage(const std::string& name, size_t bytes);
    
    /**
     * @brief 记录训练进度
     * @param epoch 当前轮次
     * @param metrics 性能指标
     */
    void record_training_progress(size_t epoch, const RegressionMetrics& metrics);
    
    /**
     * @brief 生成性能报告
     * @return 报告字符串
     */
    std::string generate_report() const;
    
    /**
     * @brief 导出性能指标
     * @param filename 文件名
     */
    void export_metrics(const std::string& filename) const;
    
private:
    std::map<std::string, Timer> timers_;
    std::map<std::string, MemoryUsage> memory_usage_;
    std::vector<TrainingProgress> training_history_;
};

} // namespace rf 
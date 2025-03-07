#include "utils/metrics.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace rf {

Metrics::Metrics() {}

Metrics::RegressionMetrics Metrics::calculate_regression_metrics(const arma::vec& y_true, 
                                                               const arma::vec& y_pred) {
    // 检查输入维度
    if (y_true.n_elem != y_pred.n_elem) {
        throw std::invalid_argument("预测值和真实值的维度不匹配");
    }

    RegressionMetrics metrics;
    
    // 计算MSE
    metrics.mse = arma::mean(arma::square(y_true - y_pred));
    
    // 计算RMSE
    metrics.rmse = std::sqrt(metrics.mse);
    
    // 计算MAE
    metrics.mae = arma::mean(arma::abs(y_true - y_pred));
    
    // 计算R²
    double y_mean = arma::mean(y_true);
    arma::vec y_mean_vec = arma::ones(y_true.n_elem) * y_mean;
    double ss_tot = arma::sum(arma::square(y_true - y_mean_vec));
    double ss_res = arma::sum(arma::square(y_true - y_pred));
    metrics.r2 = 1.0 - ss_res / ss_tot;
    
    // 计算解释方差分数
    double var_y = arma::var(y_true);
    double var_error = arma::var(y_true - y_pred);
    metrics.explained_variance = 1.0 - var_error / var_y;
    
    return metrics;
}

void Metrics::start_timer(const std::string& name) {
    timers_[name].start_time = std::chrono::high_resolution_clock::now();
}

double Metrics::stop_timer(const std::string& name) {
    auto& timer = timers_[name];
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - timer.start_time);
    
    timer.total_time += duration.count();
    timer.count++;
    
    return duration.count();
}

void Metrics::record_memory_usage(const std::string& name, size_t bytes) {
    auto& memory = memory_usage_[name];
    memory.current_usage = bytes;
    memory.peak_usage = std::max(memory.peak_usage, bytes);
    memory.history.push_back(bytes);
}

void Metrics::record_training_progress(size_t epoch, const RegressionMetrics& metrics) {
    TrainingProgress progress;
    progress.epoch = epoch;
    progress.metrics = metrics;
    progress.time = timers_["training"].total_time;
    training_history_.push_back(progress);
}

std::string Metrics::generate_report() const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    
    // 计时器报告
    ss << "性能报告\n";
    ss << "========\n\n";
    ss << "时间统计：\n";
    for (const auto& [name, timer] : timers_) {
        double avg_time = timer.total_time / timer.count;
        ss << name << "：总时间 = " << timer.total_time 
           << "ms，平均时间 = " << avg_time << "ms，调用次数 = " << timer.count << "\n";
    }
    
    // 内存使用报告
    ss << "\n内存使用：\n";
    for (const auto& [name, memory] : memory_usage_) {
        ss << name << "：当前使用 = " << memory.current_usage / 1024.0 / 1024.0 
           << "MB，峰值使用 = " << memory.peak_usage / 1024.0 / 1024.0 << "MB\n";
    }
    
    // 训练历史报告
    if (!training_history_.empty()) {
        ss << "\n训练历史：\n";
        ss << "轮次  MSE      RMSE     MAE      R²       解释方差\n";
        for (const auto& progress : training_history_) {
            ss << std::setw(6) << progress.epoch
               << std::setw(9) << progress.metrics.mse
               << std::setw(9) << progress.metrics.rmse
               << std::setw(9) << progress.metrics.mae
               << std::setw(9) << progress.metrics.r2
               << std::setw(9) << progress.metrics.explained_variance << "\n";
        }
    }
    
    return ss.str();
}

void Metrics::export_metrics(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("无法创建性能报告文件：" + filename);
    }
    
    file << generate_report();
}

} // namespace rf 
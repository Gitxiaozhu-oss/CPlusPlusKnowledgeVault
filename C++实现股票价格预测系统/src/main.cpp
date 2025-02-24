#include "arima_model.hpp"
#include "data_processor.hpp"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <cmath>

using namespace stock_predictor;

// 计算预测误差
double calculateError(const std::vector<double>& actual, const std::vector<double>& predicted) {
    if (actual.size() != predicted.size() || actual.empty()) {
        return std::numeric_limits<double>::infinity();
    }
    
    double sum_sq_error = 0.0;
    for (size_t i = 0; i < actual.size(); ++i) {
        sum_sq_error += std::pow(actual[i] - predicted[i], 2);
    }
    return std::sqrt(sum_sq_error / actual.size());  // RMSE
}

void printPredictions(const std::vector<double>& predictions,
                     const boost::posix_time::ptime& last_timestamp,
                     double mean,
                     double std) {
    std::cout << "\n预测结果：" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    for (size_t i = 0; i < predictions.size(); ++i) {
        boost::posix_time::ptime pred_time = last_timestamp + boost::posix_time::hours(24 * (i + 1));
        // 反标准化预测值
        double denormalized_price = predictions[i] * std + mean;
        std::cout << "时间: " << pred_time << ", 预测价格: " << denormalized_price << std::endl;
    }
}

// 使用交叉验证评估模型
double evaluateModel(const std::vector<double>& prices, 
                    const std::vector<boost::posix_time::ptime>& timestamps,
                    int p, int d, int q) {
    if (prices.size() < 30) return std::numeric_limits<double>::infinity();
    
    // 使用最后5个数据点作为验证集
    size_t train_size = prices.size() - 5;
    std::vector<double> train_data(prices.begin(), prices.begin() + train_size);
    std::vector<double> test_data(prices.begin() + train_size, prices.end());
    std::vector<boost::posix_time::ptime> train_timestamps(timestamps.begin(), timestamps.begin() + train_size);
    
    try {
        ARIMAModel model(p, d, q);
        model.train(train_data, train_timestamps);
        auto predictions = model.predict(5);
        return calculateError(test_data, predictions);
    } catch (const std::exception& e) {
        return std::numeric_limits<double>::infinity();
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "用法: " << argv[0] << " <数据文件路径>" << std::endl;
            return 1;
        }

        // 初始化数据处理器
        DataProcessor processor(30);
        
        // 加载数据
        std::cout << "正在加载数据..." << std::endl;
        processor.loadData(argv[1]);
        
        // 数据预处理
        std::cout << "正在预处理数据..." << std::endl;
        processor.removeOutliers();
        processor.handleMissingValues();
        processor.normalize();

        // 获取处理后的数据
        std::vector<double> prices = processor.getProcessedPrices();
        std::vector<boost::posix_time::ptime> timestamps = processor.getTimestamps();

        if (prices.empty()) {
            throw std::runtime_error("没有足够的数据进行预测");
        }

        // 尝试不同的参数组合
        std::cout << "\n正在评估不同的ARIMA模型参数组合..." << std::endl;
        int best_p = 1, best_d = 1, best_q = 1;
        double best_error = std::numeric_limits<double>::infinity();

        for (int p = 0; p <= 3; ++p) {
            for (int d = 0; d <= 2; ++d) {
                for (int q = 0; q <= 3; ++q) {
                    double error = evaluateModel(prices, timestamps, p, d, q);
                    std::cout << "ARIMA(" << p << "," << d << "," << q << ") - RMSE: ";
                    if (error == std::numeric_limits<double>::infinity()) {
                        std::cout << "无效" << std::endl;
                    } else {
                        std::cout << std::fixed << std::setprecision(4) << error << std::endl;
                        if (error < best_error) {
                            best_error = error;
                            best_p = p;
                            best_d = d;
                            best_q = q;
                        }
                    }
                }
            }
        }

        std::cout << "\n最佳模型参数: ARIMA(" << best_p << "," << best_d << "," << best_q << ")" << std::endl;
        std::cout << "最小RMSE: " << best_error << std::endl;

        // 使用最佳参数训练最终模型
        std::cout << "\n使用最佳参数训练最终模型..." << std::endl;
        ARIMAModel model(best_p, best_d, best_q);
        model.train(prices, timestamps);

        // 预测未来7天的价格
        std::cout << "正在生成预测..." << std::endl;
        std::vector<double> predictions = model.predict(7);

        // 输出预测结果
        printPredictions(predictions, timestamps.back(), processor.getMean(), processor.getStd());

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
} 
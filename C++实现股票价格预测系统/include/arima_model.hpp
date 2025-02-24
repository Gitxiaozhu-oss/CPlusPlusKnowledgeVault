#pragma once

#include <vector>
#include <Eigen/Dense>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <memory>
#include <stdexcept>

namespace stock_predictor {

class ARIMAModel {
public:
    // 构造函数，初始化ARIMA模型参数
    ARIMAModel(int p, int d, int q);

    // 训练模型
    void train(const std::vector<double>& data, 
               const std::vector<boost::posix_time::ptime>& timestamps);

    // 预测未来n步的值
    std::vector<double> predict(int n_steps);

    // 计算模型参数
    void calculateParameters();

private:
    // ARIMA模型参数
    int p_; // AR阶数
    int d_; // 差分阶数
    int q_; // MA阶数

    // 模型系数
    Eigen::VectorXd ar_coefficients_;  // AR系数 φ
    Eigen::VectorXd ma_coefficients_;  // MA系数 θ
    double c_;  // 常数项

    // 原始数据和处理后的数据
    std::vector<double> original_data_;
    std::vector<double> differenced_data_;
    std::vector<double> residuals_;

    // 辅助函数
    std::vector<double> difference(const std::vector<double>& data, int order);
    std::vector<double> inverse_difference(const std::vector<double>& diff_data, 
                                         const std::vector<double>& original_data, 
                                         int order);
    
    // 计算自相关和偏自相关
    Eigen::VectorXd calculateAutoCorrelation(const std::vector<double>& data, int max_lag);
    Eigen::VectorXd calculatePartialAutoCorrelation(const std::vector<double>& data, int max_lag);

    // 使用最小二乘法估计参数
    void estimateARMAParameters();

    // 计算均值
    double calculateMean(const std::vector<double>& data) const;
};

} // namespace stock_predictor 
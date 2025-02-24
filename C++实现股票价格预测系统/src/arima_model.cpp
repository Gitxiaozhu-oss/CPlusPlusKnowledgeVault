#include "arima_model.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iostream>

namespace stock_predictor {

ARIMAModel::ARIMAModel(int p, int d, int q)
    : p_(p), d_(d), q_(q), c_(0.0) {
    if (p < 0 || d < 0 || q < 0) {
        throw std::invalid_argument("ARIMA参数必须为非负数");
    }
}

void ARIMAModel::train(const std::vector<double>& data,
                      const std::vector<boost::posix_time::ptime>& timestamps) {
    if (data.size() != timestamps.size()) {
        throw std::invalid_argument("数据和时间戳数量不匹配");
    }

    if (data.size() < std::max(p_, q_) + d_ + 1) {
        throw std::invalid_argument("数据点数量不足");
    }

    original_data_ = data;
    
    // 执行d阶差分
    differenced_data_ = data;
    for (int i = 0; i < d_; ++i) {
        differenced_data_ = difference(differenced_data_, 1);
    }

    // 估计模型参数
    calculateParameters();
}

std::vector<double> ARIMAModel::predict(int n_steps) {
    if (n_steps <= 0) {
        throw std::invalid_argument("预测步数必须为正数");
    }

    std::vector<double> predictions(n_steps);
    std::vector<double> last_values(differenced_data_.end() - p_, differenced_data_.end());
    std::vector<double> last_errors(residuals_.end() - q_, residuals_.end());

    // 预测差分序列
    for (int i = 0; i < n_steps; ++i) {
        double prediction = c_;
        
        // AR部分
        for (int j = 0; j < p_; ++j) {
            if (j < last_values.size()) {
                prediction += ar_coefficients_(j) * last_values[last_values.size() - 1 - j];
            }
        }
        
        // MA部分
        for (int j = 0; j < q_; ++j) {
            if (j < last_errors.size()) {
                prediction += ma_coefficients_(j) * last_errors[last_errors.size() - 1 - j];
            }
        }

        predictions[i] = prediction;
        last_values.push_back(prediction);
        last_values.erase(last_values.begin());
        
        // 假设新的误差为0
        last_errors.push_back(0);
        last_errors.erase(last_errors.begin());
    }

    // 还原差分
    auto result = inverse_difference(predictions, original_data_, d_);
    
    // 验证结果
    for (auto& val : result) {
        if (std::isnan(val) || std::isinf(val)) {
            val = original_data_.back(); // 如果预测失败，使用最后一个已知值
        }
    }
    
    return result;
}

void ARIMAModel::calculateParameters() {
    // 计算自相关和偏自相关
    Eigen::VectorXd acf = calculateAutoCorrelation(differenced_data_, p_ + q_);
    Eigen::VectorXd pacf = calculatePartialAutoCorrelation(differenced_data_, p_);

    // 估计ARMA参数
    estimateARMAParameters();

    // 计算残差
    residuals_.resize(differenced_data_.size());
    for (size_t i = std::max(p_, q_); i < differenced_data_.size(); ++i) {
        double expected = c_;
        for (int j = 0; j < p_; ++j) {
            if (i > j) {
                expected += ar_coefficients_(j) * differenced_data_[i - j - 1];
            }
        }
        for (int j = 0; j < q_; ++j) {
            if (i > j) {
                expected += ma_coefficients_(j) * residuals_[i - j - 1];
            }
        }
        residuals_[i] = differenced_data_[i] - expected;
    }
}

std::vector<double> ARIMAModel::difference(const std::vector<double>& data, int order) {
    if (data.size() <= order) {
        throw std::invalid_argument("数据长度必须大于差分阶数");
    }
    
    std::vector<double> diff(data.size() - order);
    for (size_t i = order; i < data.size(); ++i) {
        diff[i - order] = data[i] - data[i - order];
    }
    return diff;
}

std::vector<double> ARIMAModel::inverse_difference(
    const std::vector<double>& diff_data,
    const std::vector<double>& original_data,
    int order) {
    
    if (original_data.size() < order) {
        throw std::invalid_argument("原始数据长度不足");
    }
    
    std::vector<double> restored = diff_data;
    for (int d = 0; d < order; ++d) {
        double base = original_data[original_data.size() - 1 - d];
        for (size_t i = 0; i < restored.size(); ++i) {
            restored[i] += base;
        }
    }
    return restored;
}

Eigen::VectorXd ARIMAModel::calculateAutoCorrelation(
    const std::vector<double>& data, int max_lag) {
    
    Eigen::VectorXd acf(max_lag + 1);
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / data.size();
    double variance = 0.0;
    
    for (const double& value : data) {
        variance += std::pow(value - mean, 2);
    }
    variance /= data.size();

    for (int lag = 0; lag <= max_lag; ++lag) {
        double sum = 0.0;
        for (size_t i = lag; i < data.size(); ++i) {
            sum += (data[i] - mean) * (data[i - lag] - mean);
        }
        acf(lag) = sum / ((data.size() - lag) * variance);
    }
    
    return acf;
}

Eigen::VectorXd ARIMAModel::calculatePartialAutoCorrelation(
    const std::vector<double>& data, int max_lag) {
    
    Eigen::VectorXd pacf(max_lag + 1);
    pacf(0) = 1.0;

    // 使用Durbin-Levinson算法计算PACF
    std::vector<std::vector<double>> phi(max_lag + 1, std::vector<double>(max_lag + 1));
    Eigen::VectorXd acf = calculateAutoCorrelation(data, max_lag);

    for (int m = 1; m <= max_lag; ++m) {
        double num = acf(m);
        double den = 1.0;
        
        for (int j = 1; j < m; ++j) {
            num -= phi[m-1][j] * acf(m-j);
            den -= phi[m-1][j] * acf(j);
        }
        
        phi[m][m] = num / den;
        pacf(m) = phi[m][m];

        for (int j = 1; j < m; ++j) {
            phi[m][j] = phi[m-1][j] - phi[m][m] * phi[m-1][m-j];
        }
    }

    return pacf;
}

double ARIMAModel::calculateMean(const std::vector<double>& data) const {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

void ARIMAModel::estimateARMAParameters() {
    // 使用Yule-Walker方程估计AR参数
    if (p_ > 0) {
        Eigen::MatrixXd R(p_, p_);
        Eigen::VectorXd r(p_);
        Eigen::VectorXd acf = calculateAutoCorrelation(differenced_data_, p_);

        for (int i = 0; i < p_; ++i) {
            r(i) = acf(i + 1);
            for (int j = 0; j < p_; ++j) {
                R(i, j) = acf(std::abs(i - j));
            }
        }

        ar_coefficients_ = R.ldlt().solve(r);
    } else {
        ar_coefficients_ = Eigen::VectorXd::Zero(0);
    }

    // 使用创新算法估计MA参数
    if (q_ > 0) {
        ma_coefficients_ = Eigen::VectorXd::Zero(q_);
        // 简化处理：使用残差的自相关作为MA系数的初始估计
        Eigen::VectorXd acf = calculateAutoCorrelation(residuals_, q_);
        for (int i = 0; i < q_; ++i) {
            ma_coefficients_(i) = acf(i + 1);
        }
    } else {
        ma_coefficients_ = Eigen::VectorXd::Zero(0);
    }

    // 估计常数项
    c_ = calculateMean(differenced_data_);
}

} // namespace stock_predictor 
#include "data_processor.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace stock_predictor {

DataProcessor::DataProcessor(size_t window_size)
    : window_size_(window_size), mean_(0.0), std_(1.0) {
    if (window_size == 0) {
        throw std::invalid_argument("窗口大小必须大于0");
    }
}

void DataProcessor::loadData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("无法打开文件: " + filename);
    }

    std::string line;
    // 跳过标题行
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string timestamp_str;
        std::string price_str, volume_str;

        if (std::getline(iss, timestamp_str, ',') &&
            std::getline(iss, price_str, ',') &&
            std::getline(iss, volume_str, ',')) {
            
            try {
                StockData data;
                data.timestamp = boost::posix_time::time_from_string(timestamp_str);
                data.price = std::stod(price_str);
                data.volume = std::stod(volume_str);

                if (isValidPrice(data.price) && isValidTimestamp(data.timestamp)) {
                    data_.push_back(data);
                }
            } catch (const std::exception& e) {
                // 记录错误但继续处理
                std::cerr << "解析错误: " << e.what() << " at line: " << line << std::endl;
            }
        }
    }

    if (data_.empty()) {
        throw std::runtime_error("没有有效数据被加载");
    }

    // 按时间戳排序
    std::sort(data_.begin(), data_.end(),
              [](const StockData& a, const StockData& b) {
                  return a.timestamp < b.timestamp;
              });
}

std::vector<double> DataProcessor::getProcessedPrices() const {
    std::vector<double> prices;
    prices.reserve(data_.size());
    for (const auto& data : data_) {
        prices.push_back(data.price);
    }
    return prices;
}

std::vector<boost::posix_time::ptime> DataProcessor::getTimestamps() const {
    std::vector<boost::posix_time::ptime> timestamps;
    timestamps.reserve(data_.size());
    for (const auto& data : data_) {
        timestamps.push_back(data.timestamp);
    }
    return timestamps;
}

void DataProcessor::normalize() {
    if (data_.empty()) return;

    std::vector<double> prices = getProcessedPrices();
    mean_ = calculateMean(prices);
    std_ = calculateStd(prices);

    if (std_ < 1e-10) {
        std_ = 1.0;  // 避免除以接近零的数
    }

    for (auto& data : data_) {
        data.price = (data.price - mean_) / std_;
    }
}

void DataProcessor::removeOutliers() {
    if (data_.empty()) return;

    std::vector<double> prices = getProcessedPrices();
    double mean = calculateMean(prices);
    double std = calculateStd(prices);

    // 使用3-sigma法则删除异常值
    data_.erase(
        std::remove_if(data_.begin(), data_.end(),
                      [mean, std](const StockData& data) {
                          return std::abs(data.price - mean) > 3 * std;
                      }),
        data_.end());
}

void DataProcessor::handleMissingValues() {
    if (data_.size() < 2) return;

    for (size_t i = 1; i < data_.size(); ++i) {
        auto time_diff = data_[i].timestamp - data_[i-1].timestamp;
        
        // 如果两个相邻数据点之间的时间差异过大，使用线性插值
        if (time_diff.hours() > 24) {
            double price_diff = data_[i].price - data_[i-1].price;
            double volume_diff = data_[i].volume - data_[i-1].volume;
            int hours = time_diff.hours();
            
            for (int h = 1; h < hours; h++) {
                StockData interpolated;
                interpolated.timestamp = data_[i-1].timestamp + boost::posix_time::hours(h);
                interpolated.price = data_[i-1].price + (price_diff * h / hours);
                interpolated.volume = data_[i-1].volume + (volume_diff * h / hours);
                
                data_.insert(data_.begin() + i, interpolated);
            }
        }
    }
}

void DataProcessor::updateWindow(const StockData& new_data) {
    if (!isValidPrice(new_data.price) || !isValidTimestamp(new_data.timestamp)) {
        throw std::invalid_argument("无效的数据点");
    }

    sliding_window_.push_back(new_data.price);
    if (sliding_window_.size() > window_size_) {
        sliding_window_.pop_front();
    }
}

std::vector<double> DataProcessor::getWindowData() const {
    return std::vector<double>(sliding_window_.begin(), sliding_window_.end());
}

double DataProcessor::calculateMean(const std::vector<double>& data) const {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double DataProcessor::calculateStd(const std::vector<double>& data) const {
    if (data.size() < 2) return 0.0;
    
    double mean = calculateMean(data);
    double sum_sq = std::accumulate(data.begin(), data.end(), 0.0,
        [mean](double acc, double val) {
            return acc + std::pow(val - mean, 2);
        });
    
    return std::sqrt(sum_sq / (data.size() - 1));
}

bool DataProcessor::isValidPrice(double price) const {
    return price > 0 && !std::isnan(price) && !std::isinf(price);
}

bool DataProcessor::isValidTimestamp(const boost::posix_time::ptime& timestamp) const {
    return !timestamp.is_not_a_date_time();
}

} // namespace stock_predictor 
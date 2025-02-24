#pragma once

#include <vector>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <deque>

namespace stock_predictor {

struct StockData {
    boost::posix_time::ptime timestamp;
    double price;
    double volume;
};

class DataProcessor {
public:
    DataProcessor(size_t window_size = 30);

    // 加载数据
    void loadData(const std::string& filename);

    // 获取处理后的时间序列数据
    std::vector<double> getProcessedPrices() const;
    std::vector<boost::posix_time::ptime> getTimestamps() const;

    // 数据预处理
    void normalize();
    void removeOutliers();
    void handleMissingValues();

    // 获取标准化参数
    double getMean() const { return mean_; }
    double getStd() const { return std_; }

    // 滑动窗口操作
    void updateWindow(const StockData& new_data);
    std::vector<double> getWindowData() const;

private:
    std::vector<StockData> data_;
    std::deque<double> sliding_window_;
    size_t window_size_;
    double mean_;  // 添加均值存储
    double std_;   // 添加标准差存储

    // 统计量计算
    double calculateMean(const std::vector<double>& data) const;
    double calculateStd(const std::vector<double>& data) const;
    
    // 数据验证
    bool isValidPrice(double price) const;
    bool isValidTimestamp(const boost::posix_time::ptime& timestamp) const;
};

} // namespace stock_predictor 
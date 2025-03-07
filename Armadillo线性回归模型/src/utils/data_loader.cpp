#include "utils/data_loader.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>

namespace rf {

DataLoader::DataLoader(arma::uword batch_size)
    : batch_size_(batch_size) {}

// BatchIterator实现
DataLoader::BatchIterator::BatchIterator(const arma::mat& X, const arma::vec& y, arma::uword batch_size)
    : X_(X)
    , y_(y)
    , batch_size_(batch_size)
    , current_pos_(0) {}

bool DataLoader::BatchIterator::has_next() const {
    return current_pos_ < X_.n_rows;
}

bool DataLoader::BatchIterator::next() {
    if (current_pos_ >= X_.n_rows) {
        return false;
    }
    
    arma::uword end_pos = std::min(current_pos_ + batch_size_, X_.n_rows);
    current_batch_ = X_.rows(current_pos_, end_pos - 1);
    current_labels_ = y_.subvec(current_pos_, end_pos - 1);
    current_pos_ = end_pos;
    
    return true;
}

std::pair<arma::mat, arma::vec> DataLoader::load_csv(const std::string& filename, bool has_header) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("无法打开文件：" + filename);
    }
    
    std::string line;
    std::vector<std::vector<double>> data;
    
    // 跳过表头
    if (has_header) {
        std::getline(file, line);
    }
    
    // 读取数据
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::vector<double> row;
        std::string value;
        
        while (std::getline(ss, value, ',')) {
            row.push_back(std::stod(value));
        }
        
        data.push_back(row);
    }
    
    // 转换为Armadillo矩阵
    arma::uword n_samples = data.size();
    arma::uword n_features = data[0].size() - 1;  // 最后一列为目标值
    
    arma::mat X(n_samples, n_features);
    arma::vec y(n_samples);
    
    for (arma::uword i = 0; i < n_samples; ++i) {
        for (arma::uword j = 0; j < n_features; ++j) {
            X(i, j) = data[i][j];
        }
        y(i) = data[i].back();
    }
    
    return {X, y};
}

std::pair<arma::mat, arma::vec> DataLoader::load_large_csv(const std::string& filename, bool has_header) {
    // 使用批处理方式加载大文件
    std::vector<arma::mat> X_batches;
    std::vector<arma::vec> y_batches;
    
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("无法打开文件：" + filename);
    }
    
    std::string line;
    std::vector<std::vector<double>> batch_data;
    
    // 跳过表头
    if (has_header) {
        std::getline(file, line);
    }
    
    arma::uword n_features = 0;
    
    // 按批次读取数据
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::vector<double> row;
        std::string value;
        
        while (std::getline(ss, value, ',')) {
            row.push_back(std::stod(value));
        }
        
        if (n_features == 0) {
            n_features = row.size() - 1;
        }
        
        batch_data.push_back(row);
        
        if (batch_data.size() >= batch_size_) {
            // 转换批次数据
            arma::mat X_batch(batch_size_, n_features);
            arma::vec y_batch(batch_size_);
            
            for (arma::uword i = 0; i < batch_size_; ++i) {
                for (arma::uword j = 0; j < n_features; ++j) {
                    X_batch(i, j) = batch_data[i][j];
                }
                y_batch(i) = batch_data[i].back();
            }
            
            X_batches.push_back(X_batch);
            y_batches.push_back(y_batch);
            batch_data.clear();
        }
    }
    
    // 处理最后一个不完整的批次
    if (!batch_data.empty()) {
        arma::uword last_batch_size = batch_data.size();
        arma::mat X_batch(last_batch_size, n_features);
        arma::vec y_batch(last_batch_size);
        
        for (arma::uword i = 0; i < last_batch_size; ++i) {
            for (arma::uword j = 0; j < n_features; ++j) {
                X_batch(i, j) = batch_data[i][j];
            }
            y_batch(i) = batch_data[i].back();
        }
        
        X_batches.push_back(X_batch);
        y_batches.push_back(y_batch);
    }
    
    // 合并所有批次
    arma::uword total_samples = 0;
    for (const auto& batch : X_batches) {
        total_samples += batch.n_rows;
    }
    
    arma::mat X(total_samples, n_features);
    arma::vec y(total_samples);
    
    arma::uword current_row = 0;
    for (arma::uword i = 0; i < X_batches.size(); ++i) {
        arma::uword batch_rows = X_batches[i].n_rows;
        X.rows(current_row, current_row + batch_rows - 1) = X_batches[i];
        y.subvec(current_row, current_row + batch_rows - 1) = y_batches[i];
        current_row += batch_rows;
    }
    
    return {X, y};
}

DataLoader::BatchIterator DataLoader::get_batch_iterator(const arma::mat& X, const arma::vec& y) {
    return BatchIterator(X, y, batch_size_);
}

arma::mat DataLoader::standardize(const arma::mat& X) {
    mean_ = arma::mean(X, 0).t();
    std_ = arma::stddev(X, 0).t();
    
    return (X - arma::repmat(mean_.t(), X.n_rows, 1)) / 
           arma::repmat(std_.t(), X.n_rows, 1);
}

arma::mat DataLoader::normalize(const arma::mat& X) {
    min_ = arma::min(X, 0).t();
    max_ = arma::max(X, 0).t();
    
    return (X - arma::repmat(min_.t(), X.n_rows, 1)) / 
           arma::repmat((max_ - min_).t(), X.n_rows, 1);
}

arma::mat DataLoader::handle_missing_values(const arma::mat& X, const std::string& strategy) {
    arma::mat X_cleaned = X;
    
    for (arma::uword j = 0; j < X.n_cols; ++j) {
        arma::uvec missing = arma::find_nonfinite(X.col(j));
        
        if (missing.n_elem > 0) {
            double fill_value;
            
            if (strategy == "mean") {
                fill_value = arma::mean(X.col(j));
            } else if (strategy == "median") {
                fill_value = arma::median(X.col(j));
            } else if (strategy == "mode") {
                // 简单实现：使用最频繁出现的值
                std::vector<double> values(X.col(j).begin(), X.col(j).end());
                std::sort(values.begin(), values.end());
                arma::uword max_count = 0;
                double mode = values[0];
                arma::uword current_count = 1;
                
                for (arma::uword i = 1; i < values.size(); ++i) {
                    if (values[i] == values[i-1]) {
                        current_count++;
                    } else {
                        if (current_count > max_count) {
                            max_count = current_count;
                            mode = values[i-1];
                        }
                        current_count = 1;
                    }
                }
                
                fill_value = mode;
            } else {
                throw std::invalid_argument("无效的缺失值处理策略：" + strategy);
            }
            
            X_cleaned.elem(missing + j * X.n_rows).fill(fill_value);
        }
    }
    
    return X_cleaned;
}

} // namespace rf 
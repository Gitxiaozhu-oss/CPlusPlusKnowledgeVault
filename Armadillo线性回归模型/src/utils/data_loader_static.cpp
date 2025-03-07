#include "utils/data_loader.hpp"
#include <curl/curl.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <memory>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>

namespace {
// 用于curl写入数据的回调函数
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// RAII文件句柄包装器
class FileHandle {
public:
    explicit FileHandle(const char* path, const char* mode) : fp_(fopen(path, mode)) {
        if (!fp_) {
            throw std::runtime_error(std::string("无法打开文件：") + path);
        }
    }
    
    ~FileHandle() {
        if (fp_) {
            fclose(fp_);
        }
    }
    
    FILE* get() { return fp_; }
    
private:
    FILE* fp_;
};

// RAII CURL句柄包装器
class CurlHandle {
public:
    CurlHandle() : curl_(curl_easy_init()) {
        if (!curl_) {
            throw std::runtime_error("无法初始化CURL");
        }
    }
    
    ~CurlHandle() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
    }
    
    CURL* get() { return curl_; }
    
private:
    CURL* curl_;
};
} // anonymous namespace

namespace rf {

std::pair<arma::mat, arma::vec> DataLoader::load_california_housing(const std::string& save_path) {
    try {
        // 创建data目录（如果不存在）
        #ifdef _WIN32
        system("mkdir data 2> nul");
        #else
        system("mkdir -p data");
        #endif
        
        // 生成模拟数据
        const size_t n_samples = 1000;
        const size_t n_features = 8;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<double> normal(0.0, 1.0);
        std::uniform_real_distribution<double> uniform(0.0, 1.0);
        
        arma::mat X(n_samples, n_features);
        arma::vec y(n_samples);
        
        // 生成特征
        for (size_t i = 0; i < n_samples; ++i) {
            // MedInc: 收入中位数
            X(i, 0) = std::abs(normal(gen)) * 10;
            
            // HouseAge: 房屋年龄
            X(i, 1) = uniform(gen) * 50;
            
            // AveRooms: 平均房间数
            X(i, 2) = normal(gen) * 2 + 6;
            
            // AveBedrms: 平均卧室数
            X(i, 3) = normal(gen) + 3;
            
            // Population: 人口
            X(i, 4) = std::abs(normal(gen)) * 1000 + 1000;
            
            // AveOccup: 平均入住率
            X(i, 5) = normal(gen) * 0.5 + 2.5;
            
            // Latitude: 纬度
            X(i, 6) = normal(gen) * 2 + 35;
            
            // Longitude: 经度
            X(i, 7) = normal(gen) * 2 - 120;
            
            // 生成目标值（房价）
            y(i) = 0.7 * X(i, 0)                    // 收入的影响
                   - 0.1 * X(i, 1)                  // 房龄的负面影响
                   + 0.3 * X(i, 2)                  // 房间数的影响
                   + 0.2 * X(i, 3)                  // 卧室数的影响
                   - 0.1 * std::log(X(i, 4))        // 人口的对数负面影响
                   + 0.1 * X(i, 5)                  // 入住率的影响
                   + normal(gen) * 0.5;             // 随机噪声
            
            // 确保房价为正值
            y(i) = std::abs(y(i)) * 100000;
        }
        
        // 保存数据到CSV文件
        std::ofstream file(save_path);
        if (!file) {
            throw std::runtime_error("无法创建文件：" + save_path);
        }
        
        // 写入表头
        file << "MedInc,HouseAge,AveRooms,AveBedrms,Population,AveOccup,Latitude,Longitude,Price\n";
        
        // 写入数据
        for (size_t i = 0; i < n_samples; ++i) {
            for (size_t j = 0; j < n_features; ++j) {
                file << X(i, j) << ",";
            }
            file << y(i) << "\n";
        }
        
        // 标准化特征
        DataLoader loader;
        X = loader.standardize(X);
        
        return {X, y};
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("加载加州房价数据集失败：") + e.what());
    }
}

} // namespace rf 
#include "SVD.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

SVD::SVD(const std::map<int, std::map<int, double>>& ratingMatrix,
         int numFactors,
         int numUsers,
         int numItems)
    : ratingMatrix_(ratingMatrix),
      numFactors_(numFactors),
      numUsers_(numUsers),
      numItems_(numItems),
      generator_(std::random_device()()),
      distribution_(0.0, 0.1) {
    
    createIdMappings();
    initializeParameters();
    computeGlobalMean();
}

void SVD::createIdMappings() {
    // 创建用户ID映射
    int userIndex = 0;
    for (const auto& userRatings : ratingMatrix_) {
        if (userIdMap_.find(userRatings.first) == userIdMap_.end()) {
            userIdMap_[userRatings.first] = userIndex++;
        }
    }
    
    // 创建电影ID映射
    int movieIndex = 0;
    for (const auto& userRatings : ratingMatrix_) {
        for (const auto& rating : userRatings.second) {
            if (movieIdMap_.find(rating.first) == movieIdMap_.end()) {
                movieIdMap_[rating.first] = movieIndex++;
            }
        }
    }
    
    // 更新实际的用户和电影数量
    numUsers_ = userIdMap_.size();
    numItems_ = movieIdMap_.size();
}

int SVD::getUserIndex(int userId) const {
    auto it = userIdMap_.find(userId);
    if (it == userIdMap_.end()) {
        throw std::runtime_error("未知的用户ID: " + std::to_string(userId));
    }
    return it->second;
}

int SVD::getMovieIndex(int movieId) const {
    auto it = movieIdMap_.find(movieId);
    if (it == movieIdMap_.end()) {
        throw std::runtime_error("未知的电影ID: " + std::to_string(movieId));
    }
    return it->second;
}

void SVD::initializeParameters() {
    // 初始化用户特征矩阵
    userFeatures_ = std::vector<std::vector<double>>(
        numUsers_, std::vector<double>(numFactors_));
    
    // 初始化电影特征矩阵
    itemFeatures_ = std::vector<std::vector<double>>(
        numItems_, std::vector<double>(numFactors_));
    
    // 初始化偏置项
    userBias_ = std::vector<double>(numUsers_, 0.0);
    itemBias_ = std::vector<double>(numItems_, 0.0);
    
    // 随机初始化特征向量
    for (int u = 0; u < numUsers_; ++u) {
        for (int f = 0; f < numFactors_; ++f) {
            userFeatures_[u][f] = distribution_(generator_);
        }
    }
    
    for (int i = 0; i < numItems_; ++i) {
        for (int f = 0; f < numFactors_; ++f) {
            itemFeatures_[i][f] = distribution_(generator_);
        }
    }
}

void SVD::computeGlobalMean() {
    double sum = 0.0;
    int count = 0;
    
    for (const auto& userRatings : ratingMatrix_) {
        for (const auto& rating : userRatings.second) {
            sum += rating.second;
            ++count;
        }
    }
    
    globalMean_ = count > 0 ? sum / count : 0.0;
}

void SVD::train(int numIterations, double learningRate, double regularization) {
    for (int iter = 0; iter < numIterations; ++iter) {
        double error = 0.0;
        int count = 0;
        
        // 对每个用户的每个评分进行训练
        for (const auto& userRatings : ratingMatrix_) {
            int userId = userRatings.first;
            int userIndex = getUserIndex(userId);
            
            for (const auto& rating : userRatings.second) {
                int movieId = rating.first;
                int movieIndex = getMovieIndex(movieId);
                double actualRating = rating.second;
                
                // 计算预测评分
                double predictedRating = predict(userId, movieId);
                
                // 计算误差
                double err = actualRating - predictedRating;
                error += err * err;
                ++count;
                
                // 更新偏置项
                userBias_[userIndex] += learningRate * (err - regularization * userBias_[userIndex]);
                itemBias_[movieIndex] += learningRate * (err - regularization * itemBias_[movieIndex]);
                
                // 更新特征向量
                for (int f = 0; f < numFactors_; ++f) {
                    double userFeature = userFeatures_[userIndex][f];
                    double itemFeature = itemFeatures_[movieIndex][f];
                    
                    userFeatures_[userIndex][f] += learningRate * (err * itemFeature - 
                                                 regularization * userFeature);
                    itemFeatures_[movieIndex][f] += learningRate * (err * userFeature - 
                                                  regularization * itemFeature);
                }
            }
        }
        
        // 计算RMSE
        if (count > 0) {
            double rmse = std::sqrt(error / count);
            if (iter % 10 == 0) {
                std::cout << "迭代 " << iter << ", RMSE: " << rmse << std::endl;
            }
        }
    }
}

double SVD::predict(int userId, int movieId) const {
    int userIndex = getUserIndex(userId);
    int movieIndex = getMovieIndex(movieId);
    
    double prediction = globalMean_;
    
    // 添加偏置项
    prediction += userBias_[userIndex] + itemBias_[movieIndex];
    
    // 计算用户特征向量和电影特征向量的点积
    for (int f = 0; f < numFactors_; ++f) {
        prediction += userFeatures_[userIndex][f] * itemFeatures_[movieIndex][f];
    }
    
    // 将预测值限制在合理范围内（例如1-5分）
    prediction = std::max(1.0, std::min(5.0, prediction));
    
    return prediction;
}

std::vector<double> SVD::getUserFeatures(int userId) const {
    int userIndex = getUserIndex(userId);
    return userFeatures_[userIndex];
}

std::vector<double> SVD::getItemFeatures(int movieId) const {
    int movieIndex = getMovieIndex(movieId);
    return itemFeatures_[movieIndex];
} 
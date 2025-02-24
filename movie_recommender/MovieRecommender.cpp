#include "MovieRecommender.h"
#include <algorithm>
#include <set>

MovieRecommender::MovieRecommender(const std::string& dbPath, int numFactors)
    : numFactors_(numFactors) {
    // 创建数据库连接
    db_ = std::make_unique<Database>(dbPath);
    loadRatings();
}

void MovieRecommender::loadRatings() {
    // 从数据库加载所有评分
    auto ratings = db_->getAllRatings();
    
    // 构建评分矩阵
    std::set<int> userIds;
    std::set<int> movieIds;
    
    for (const auto& rating : ratings) {
        ratingMatrix_[rating.userId][rating.movieId] = rating.rating;
        userIds.insert(rating.userId);
        movieIds.insert(rating.movieId);
    }
    
    // 创建SVD模型
    svd_ = std::make_unique<SVD>(ratingMatrix_,
                                numFactors_,
                                userIds.size(),
                                movieIds.size());
}

void MovieRecommender::addRating(int userId, int movieId, double rating) {
    // 添加到数据库
    db_->addRating(userId, movieId, rating);
    
    // 更新评分矩阵
    ratingMatrix_[userId][movieId] = rating;
    
    // 重新加载数据并训练模型
    loadRatings();
}

void MovieRecommender::trainModel(int numIterations, double learningRate, double regularization) {
    if (svd_) {
        svd_->train(numIterations, learningRate, regularization);
    }
}

double MovieRecommender::predictRating(int userId, int movieId) {
    if (svd_) {
        return svd_->predict(userId, movieId);
    }
    return 0.0;
}

std::vector<std::pair<int, double>> MovieRecommender::getRecommendations(int userId, int topN) {
    std::vector<std::pair<int, double>> predictions;
    
    // 获取用户已经评分的电影
    std::set<int> ratedMovies;
    if (ratingMatrix_.count(userId)) {
        for (const auto& rating : ratingMatrix_[userId]) {
            ratedMovies.insert(rating.first);
        }
    }
    
    // 获取所有可能的电影ID
    std::set<int> allMovieIds;
    for (const auto& userRatings : ratingMatrix_) {
        for (const auto& rating : userRatings.second) {
            allMovieIds.insert(rating.first);
        }
    }
    
    // 对所有未评分的电影进行预测
    for (int movieId : allMovieIds) {
        if (ratedMovies.count(movieId) == 0) {
            try {
                double predictedRating = predictRating(userId, movieId);
                predictions.emplace_back(movieId, predictedRating);
            } catch (const std::exception& e) {
                // 如果预测失败，跳过这个电影
                continue;
            }
        }
    }
    
    // 排序并返回前topN个推荐
    std::sort(predictions.begin(), predictions.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    if (predictions.size() > topN) {
        predictions.resize(topN);
    }
    
    return predictions;
}

void MovieRecommender::preprocessData() {
    // 这里可以添加数据预处理的步骤
    // 例如：归一化、处理缺失值等
} 
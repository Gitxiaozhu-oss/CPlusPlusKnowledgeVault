#ifndef MOVIE_RECOMMENDER_H
#define MOVIE_RECOMMENDER_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "Database.h"
#include "SVD.h"

class MovieRecommender {
public:
    // 构造函数
    MovieRecommender(const std::string& dbPath, int numFactors = 100);
    
    // 添加用户评分
    void addRating(int userId, int movieId, double rating);
    
    // 训练模型
    void trainModel(int numIterations = 100, double learningRate = 0.005, double regularization = 0.02);
    
    // 获取电影推荐
    std::vector<std::pair<int, double>> getRecommendations(int userId, int topN = 10);
    
    // 预测用户对电影的评分
    double predictRating(int userId, int movieId);

private:
    std::unique_ptr<Database> db_;
    std::unique_ptr<SVD> svd_;
    int numFactors_;
    
    // 用户-电影评分矩阵
    std::map<int, std::map<int, double>> ratingMatrix_;
    
    // 加载评分数据
    void loadRatings();
    
    // 预处理数据
    void preprocessData();
};

#endif // MOVIE_RECOMMENDER_H 
#include "MovieRecommender.h"
#include <iostream>
#include <iomanip>

void printRecommendations(const std::vector<std::pair<int, double>>& recommendations) {
    std::cout << "\n推荐电影列表：" << std::endl;
    std::cout << std::setw(10) << "电影ID" << std::setw(15) << "预测评分" << std::endl;
    std::cout << std::string(25, '-') << std::endl;
    
    for (const auto& rec : recommendations) {
        std::cout << std::setw(10) << rec.first 
                  << std::setw(15) << std::fixed << std::setprecision(2) << rec.second 
                  << std::endl;
    }
}

int main() {
    try {
        // 创建推荐系统实例
        MovieRecommender recommender("movies.db", 100);
        
        // 添加一些示例评分数据
        std::cout << "添加示例评分数据..." << std::endl;
        
        // 用户1的评分
        recommender.addRating(1, 1, 5.0);  // 电影1
        recommender.addRating(1, 2, 3.5);  // 电影2
        recommender.addRating(1, 3, 4.0);  // 电影3
        
        // 用户2的评分
        recommender.addRating(2, 1, 3.0);
        recommender.addRating(2, 2, 4.0);
        recommender.addRating(2, 4, 5.0);
        
        // 用户3的评分
        recommender.addRating(3, 1, 4.0);
        recommender.addRating(3, 3, 3.5);
        recommender.addRating(3, 4, 4.0);
        
        // 训练模型
        std::cout << "训练推荐模型..." << std::endl;
        recommender.trainModel(100, 0.005, 0.02);
        
        // 为用户1获取推荐
        std::cout << "\n为用户1生成推荐..." << std::endl;
        auto recommendations = recommender.getRecommendations(1, 5);
        printRecommendations(recommendations);
        
        // 预测特定评分
        int userId = 1;
        int movieId = 4;
        double predictedRating = recommender.predictRating(userId, movieId);
        std::cout << "\n预测用户 " << userId << " 对电影 " << movieId 
                  << " 的评分: " << std::fixed << std::setprecision(2) 
                  << predictedRating << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 
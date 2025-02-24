#ifndef SVD_H
#define SVD_H

#include <vector>
#include <map>
#include <random>
#include <cmath>
#include <unordered_map>

class SVD {
public:
    // 构造函数
    SVD(const std::map<int, std::map<int, double>>& ratingMatrix, 
        int numFactors,
        int numUsers,
        int numItems);

    // 训练模型
    void train(int numIterations, double learningRate, double regularization);

    // 预测评分
    double predict(int userId, int movieId) const;

    // 获取用户特征向量
    std::vector<double> getUserFeatures(int userId) const;

    // 获取电影特征向量
    std::vector<double> getItemFeatures(int movieId) const;

private:
    int numFactors_;
    int numUsers_;
    int numItems_;
    
    // ID映射
    std::unordered_map<int, int> userIdMap_;    // 外部ID到内部索引的映射
    std::unordered_map<int, int> movieIdMap_;   // 外部ID到内部索引的映射
    
    // 用户-电影评分矩阵
    std::map<int, std::map<int, double>> ratingMatrix_;
    
    // 用户特征矩阵
    std::vector<std::vector<double>> userFeatures_;
    
    // 电影特征矩阵
    std::vector<std::vector<double>> itemFeatures_;
    
    // 用户偏置
    std::vector<double> userBias_;
    
    // 电影偏置
    std::vector<double> itemBias_;
    
    // 全局平均评分
    double globalMean_;

    // 初始化模型参数
    void initializeParameters();
    
    // 计算全局平均评分
    void computeGlobalMean();
    
    // 创建ID映射
    void createIdMappings();
    
    // 获取内部用户索引
    int getUserIndex(int userId) const;
    
    // 获取内部电影索引
    int getMovieIndex(int movieId) const;
    
    // 随机数生成器
    std::default_random_engine generator_;
    std::normal_distribution<double> distribution_;
};

#endif // SVD_H 
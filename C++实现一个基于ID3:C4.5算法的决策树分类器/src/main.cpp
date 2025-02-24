#include "DecisionTree.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <algorithm>

// 加载鸢尾花数据集
bool loadIrisData(
    const std::string& filename,
    std::vector<std::vector<double>>& data,
    std::vector<std::string>& labels) {
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        std::vector<double> sample;
        
        // 读取4个特征值
        for (int i = 0; i < 4; ++i) {
            std::getline(ss, value, ',');
            sample.push_back(std::stod(value));
        }
        
        // 读取类别标签
        std::getline(ss, value, ',');
        
        data.push_back(sample);
        labels.push_back(value);
    }
    
    return true;
}

// 将数据集随机分为训练集和测试集
void splitDataset(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& labels,
    double trainRatio,
    std::vector<std::vector<double>>& trainData,
    std::vector<std::string>& trainLabels,
    std::vector<std::vector<double>>& testData,
    std::vector<std::string>& testLabels) {
    
    std::vector<size_t> indices(data.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i] = i;
    }
    
    // 随机打乱索引
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(indices.begin(), indices.end(), gen);
    
    size_t trainSize = static_cast<size_t>(data.size() * trainRatio);
    
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i < trainSize) {
            trainData.push_back(data[indices[i]]);
            trainLabels.push_back(labels[indices[i]]);
        } else {
            testData.push_back(data[indices[i]]);
            testLabels.push_back(labels[indices[i]]);
        }
    }
}

int main() {
    // 加载数据
    std::vector<std::vector<double>> data;
    std::vector<std::string> labels;
    
    if (!loadIrisData("iris.csv", data, labels)) {
        return 1;
    }
    
    // 分割数据集
    std::vector<std::vector<double>> trainData, testData;
    std::vector<std::string> trainLabels, testLabels;
    splitDataset(data, labels, 0.7, trainData, trainLabels, testData, testLabels);
    
    // 创建并训练决策树（使用C4.5算法）
    DecisionTree tree(true, 5);
    tree.train(trainData, trainLabels);
    
    // 导出决策树可视化
    tree.exportToDot("decision_tree.dot");
    std::cout << "决策树已导出到 decision_tree.dot" << std::endl;
    
    // 在测试集上进行预测
    int correct = 0;
    for (size_t i = 0; i < testData.size(); ++i) {
        std::string predicted = tree.predict(testData[i]);
        if (predicted == testLabels[i]) {
            correct++;
        }
        std::cout << "样本 " << i << ": 预测=" << predicted 
                  << ", 实际=" << testLabels[i] << std::endl;
    }
    
    // 计算准确率
    double accuracy = static_cast<double>(correct) / testData.size();
    std::cout << "测试集准确率: " << accuracy * 100 << "%" << std::endl;
    
    return 0;
} 
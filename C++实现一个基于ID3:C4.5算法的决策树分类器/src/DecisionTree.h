#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#include <vector>
#include <string>
#include <memory>
#include "Node.h"

class DecisionTree {
public:
    DecisionTree(bool useC45 = false, int maxDepth = 10);
    ~DecisionTree();

    // 训练决策树
    void train(const std::vector<std::vector<double>>& data, 
               const std::vector<std::string>& labels);
    
    // 预测单个样本
    std::string predict(const std::vector<double>& sample) const;
    
    // 导出决策树为DOT格式
    void exportToDot(const std::string& filename) const;

private:
    std::shared_ptr<Node> root;
    bool useC45;        // 是否使用C4.5算法（否则使用ID3）
    int maxDepth;       // 最大树深度

    // 递归构建决策树
    std::shared_ptr<Node> buildTree(
        const std::vector<std::vector<double>>& data,
        const std::vector<std::string>& labels,
        const std::vector<int>& availableFeatures,
        int depth
    );

    // 计算信息熵
    double calculateEntropy(const std::vector<std::string>& labels) const;
    
    // 计算信息增益
    double calculateInformationGain(
        const std::vector<std::vector<double>>& data,
        const std::vector<std::string>& labels,
        int featureIndex
    ) const;

    // 计算信息增益率（C4.5算法使用）
    double calculateInformationGainRatio(
        const std::vector<std::vector<double>>& data,
        const std::vector<std::string>& labels,
        int featureIndex
    ) const;

    // 递归生成DOT格式字符串
    void generateDot(std::shared_ptr<Node> node, std::string& dot, int& nodeCount) const;
};

#endif // DECISION_TREE_H 
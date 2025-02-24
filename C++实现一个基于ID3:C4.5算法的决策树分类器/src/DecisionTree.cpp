#include "DecisionTree.h"
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

DecisionTree::DecisionTree(bool useC45, int maxDepth) 
    : useC45(useC45), maxDepth(maxDepth), root(nullptr) {}

DecisionTree::~DecisionTree() {}

void DecisionTree::train(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& labels) {
    
    std::vector<int> availableFeatures;
    for (size_t i = 0; i < data[0].size(); ++i) {
        availableFeatures.push_back(i);
    }
    
    root = buildTree(data, labels, availableFeatures, 0);
}

std::shared_ptr<Node> DecisionTree::buildTree(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& labels,
    const std::vector<int>& availableFeatures,
    int depth) {
    
    auto node = std::make_shared<Node>();
    
    // 检查是否所有样本属于同一类别
    std::set<std::string> uniqueLabels(labels.begin(), labels.end());
    if (uniqueLabels.size() == 1) {
        node->isLeaf = true;
        node->className = *uniqueLabels.begin();
        return node;
    }
    
    // 检查是否达到最大深度或没有可用特征
    if (depth >= maxDepth || availableFeatures.empty()) {
        node->isLeaf = true;
        // 选择最多的类别作为叶子节点的类别
        std::map<std::string, int> labelCount;
        for (const auto& label : labels) {
            labelCount[label]++;
        }
        node->className = std::max_element(
            labelCount.begin(), labelCount.end(),
            [](const auto& p1, const auto& p2) {
                return p1.second < p2.second;
            }
        )->first;
        return node;
    }
    
    // 选择最佳分裂特征
    double bestGain = -1;
    int bestFeature = -1;
    double bestSplitValue = 0;
    
    for (int feature : availableFeatures) {
        double gain;
        if (useC45) {
            gain = calculateInformationGainRatio(data, labels, feature);
        } else {
            gain = calculateInformationGain(data, labels, feature);
        }
        
        if (gain > bestGain) {
            bestGain = gain;
            bestFeature = feature;
            
            // 计算最佳分裂值（取该特征的平均值）
            double sum = 0;
            for (const auto& sample : data) {
                sum += sample[feature];
            }
            bestSplitValue = sum / data.size();
        }
    }
    
    if (bestFeature == -1) {
        node->isLeaf = true;
        // 选择最多的类别作为叶子节点的类别
        std::map<std::string, int> labelCount;
        for (const auto& label : labels) {
            labelCount[label]++;
        }
        node->className = std::max_element(
            labelCount.begin(), labelCount.end(),
            [](const auto& p1, const auto& p2) {
                return p1.second < p2.second;
            }
        )->first;
        return node;
    }
    
    // 根据最佳特征分裂数据
    node->attributeIndex = bestFeature;
    node->splitValue = bestSplitValue;
    
    std::vector<std::vector<double>> leftData, rightData;
    std::vector<std::string> leftLabels, rightLabels;
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i][bestFeature] <= bestSplitValue) {
            leftData.push_back(data[i]);
            leftLabels.push_back(labels[i]);
        } else {
            rightData.push_back(data[i]);
            rightLabels.push_back(labels[i]);
        }
    }
    
    // 创建子节点
    std::vector<int> newFeatures = availableFeatures;
    newFeatures.erase(
        std::remove(newFeatures.begin(), newFeatures.end(), bestFeature),
        newFeatures.end()
    );
    
    if (!leftData.empty()) {
        auto leftChild = buildTree(leftData, leftLabels, newFeatures, depth + 1);
        node->children.push_back(leftChild);
    }
    
    if (!rightData.empty()) {
        auto rightChild = buildTree(rightData, rightLabels, newFeatures, depth + 1);
        node->children.push_back(rightChild);
    }
    
    return node;
}

double DecisionTree::calculateEntropy(const std::vector<std::string>& labels) const {
    std::map<std::string, int> labelCount;
    for (const auto& label : labels) {
        labelCount[label]++;
    }
    
    double entropy = 0.0;
    double n = labels.size();
    
    for (const auto& [label, count] : labelCount) {
        double p = count / n;
        entropy -= p * std::log2(p);
    }
    
    return entropy;
}

double DecisionTree::calculateInformationGain(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& labels,
    int featureIndex) const {
    
    double entropyBefore = calculateEntropy(labels);
    
    // 计算分裂值（取平均值）
    double splitValue = 0;
    for (const auto& sample : data) {
        splitValue += sample[featureIndex];
    }
    splitValue /= data.size();
    
    std::vector<std::string> leftLabels, rightLabels;
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i][featureIndex] <= splitValue) {
            leftLabels.push_back(labels[i]);
        } else {
            rightLabels.push_back(labels[i]);
        }
    }
    
    double entropyAfter = 
        (leftLabels.size() / static_cast<double>(labels.size())) * calculateEntropy(leftLabels) +
        (rightLabels.size() / static_cast<double>(labels.size())) * calculateEntropy(rightLabels);
    
    return entropyBefore - entropyAfter;
}

double DecisionTree::calculateInformationGainRatio(
    const std::vector<std::vector<double>>& data,
    const std::vector<std::string>& labels,
    int featureIndex) const {
    
    double infoGain = calculateInformationGain(data, labels, featureIndex);
    
    // 计算分裂信息
    double splitValue = 0;
    for (const auto& sample : data) {
        splitValue += sample[featureIndex];
    }
    splitValue /= data.size();
    
    int leftCount = 0, rightCount = 0;
    for (const auto& sample : data) {
        if (sample[featureIndex] <= splitValue) {
            leftCount++;
        } else {
            rightCount++;
        }
    }
    
    double splitInfo = 0.0;
    double n = data.size();
    
    if (leftCount > 0) {
        double p1 = leftCount / n;
        splitInfo -= p1 * std::log2(p1);
    }
    if (rightCount > 0) {
        double p2 = rightCount / n;
        splitInfo -= p2 * std::log2(p2);
    }
    
    return splitInfo == 0 ? 0 : infoGain / splitInfo;
}

std::string DecisionTree::predict(const std::vector<double>& sample) const {
    if (!root) {
        return "";
    }
    
    std::shared_ptr<Node> current = root;
    while (!current->isLeaf) {
        if (sample[current->attributeIndex] <= current->splitValue) {
            if (!current->children.empty()) {
                current = current->children[0];
            } else {
                break;
            }
        } else {
            if (current->children.size() > 1) {
                current = current->children[1];
            } else {
                break;
            }
        }
    }
    
    return current->className;
}

void DecisionTree::exportToDot(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }
    
    out << "digraph DecisionTree {\n";
    int nodeCount = 0;
    std::string dot;
    generateDot(root, dot, nodeCount);
    out << dot;
    out << "}\n";
    out.close();
}

void DecisionTree::generateDot(
    std::shared_ptr<Node> node,
    std::string& dot,
    int& nodeCount) const {
    
    if (!node) return;
    
    int currentNode = nodeCount++;
    
    if (node->isLeaf) {
        dot += "    node" + std::to_string(currentNode) + 
               " [label=\"" + node->className + "\"];\n";
    } else {
        dot += "    node" + std::to_string(currentNode) + 
               " [label=\"特征 " + std::to_string(node->attributeIndex) + 
               " <= " + std::to_string(node->splitValue) + "\"];\n";
    }
    
    for (size_t i = 0; i < node->children.size(); ++i) {
        int childNode = nodeCount;
        generateDot(node->children[i], dot, nodeCount);
        dot += "    node" + std::to_string(currentNode) + 
               " -> node" + std::to_string(childNode) + 
               " [label=\"" + (i == 0 ? "是" : "否") + "\"];\n";
    }
} 
#pragma once

#include "tree_node.hpp"
#include <random>
#include <vector>

namespace rf {

/**
 * @brief 自适应决策树类
 * 
 * 实现了动态深度的CART回归树，支持在线学习和自适应特征选择
 */
class AdaptiveTree {
public:
    /**
     * @brief 构造函数
     * @param max_depth 最大树深度
     * @param min_samples_split 最小分裂样本数
     * @param lambda 复杂度惩罚系数
     */
    AdaptiveTree(size_t max_depth = 10,
                size_t min_samples_split = 2,
                double lambda = 0.1);

    /**
     * @brief 训练模型
     * @param X 训练数据
     * @param y 目标值
     * @param feature_subset_size 特征子集大小
     */
    void fit(const arma::mat& X, const arma::vec& y, size_t feature_subset_size = 0);

    /**
     * @brief 预测
     * @param X 测试数据
     * @return 预测值
     */
    arma::vec predict(const arma::mat& X) const;

    /**
     * @brief 在线更新模型
     * @param X_new 新数据
     * @param y_new 新数据的目标值
     */
    void update(const arma::mat& X_new, const arma::vec& y_new);

    /**
     * @brief 获取特征重要性
     * @return 特征重要性向量
     */
    arma::vec get_feature_importance() const;

    /**
     * @brief 获取根节点
     * @return 根节点指针
     */
    TreeNode::NodePtr get_root() const { return root_; }

    /**
     * @brief 设置根节点
     * @param node 根节点指针
     */
    void set_root(const TreeNode::NodePtr& node) { root_ = node; }

private:
    size_t max_depth_;            ///< 最大树深度
    size_t min_samples_split_;    ///< 最小分裂样本数
    double lambda_;               ///< 复杂度惩罚系数
    std::mt19937 rng_;           ///< 随机数生成器
    TreeNode::NodePtr root_;      ///< 根节点
    std::vector<double> feature_importance_;  ///< 特征重要性

    /**
     * @brief 构建决策树
     * @param X 训练数据
     * @param y 目标值
     * @param depth 当前深度
     * @param available_features 可用特征索引
     * @return 树节点指针
     */
    TreeNode::NodePtr build_tree(const arma::mat& X,
                                const arma::vec& y,
                                size_t depth,
                                const std::vector<size_t>& available_features);

    /**
     * @brief 寻找最佳分割点
     * @param X 训练数据
     * @param y 目标值
     * @param available_features 可用特征索引
     * @return {最佳特征索引, 最佳阈值}
     */
    std::pair<size_t, double> find_best_split(const arma::mat& X,
                                             const arma::vec& y,
                                             const std::vector<size_t>& available_features);

    /**
     * @brief 计算分割增益
     * @param y_left 左子节点目标值
     * @param y_right 右子节点目标值
     * @return 分割增益
     */
    double calculate_split_gain(const arma::vec& y_left, const arma::vec& y_right) const;

    /**
     * @brief 检查是否应该剪枝
     * @param node 当前节点
     * @param X 验证数据
     * @param y 验证数据目标值
     * @return 是否应该剪枝
     */
    bool should_prune(const TreeNode::NodePtr& node,
                     const arma::mat& X,
                     const arma::vec& y) const;
};

} // namespace rf 
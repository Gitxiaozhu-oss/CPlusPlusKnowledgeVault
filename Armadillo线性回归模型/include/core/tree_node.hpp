#pragma once

#include <armadillo>
#include <memory>

namespace rf {

/**
 * @brief 决策树节点类
 * 
 * 实现了自适应决策树的节点结构，支持动态特征选择和在线更新
 */
class TreeNode {
public:
    using NodePtr = std::shared_ptr<TreeNode>;
    
    /**
     * @brief 节点类型枚举
     */
    enum class NodeType {
        LEAF,      ///< 叶节点
        INTERNAL   ///< 内部节点
    };

    /**
     * @brief 构造叶节点
     * @param value 节点预测值
     */
    explicit TreeNode(double value);

    /**
     * @brief 构造内部节点
     * @param feature_idx 分割特征索引
     * @param threshold 分割阈值
     */
    TreeNode(size_t feature_idx, double threshold);

    /**
     * @brief 预测单个样本
     * @param x 输入特征向量
     * @return 预测值
     */
    double predict(const arma::vec& x) const;

    /**
     * @brief 更新节点统计信息
     * @param x 输入特征向量
     * @param y 目标值
     */
    void update_stats(const arma::vec& x, double y);

    /**
     * @brief 检查是否应该分裂节点
     * @param min_samples_split 最小分裂样本数
     * @param max_depth 最大深度
     * @param current_depth 当前深度
     * @return 是否应该分裂
     */
    bool should_split(size_t min_samples_split, size_t max_depth, size_t current_depth) const;

    // Getters
    NodeType get_type() const { return type_; }
    size_t get_feature_idx() const { return feature_idx_; }
    double get_threshold() const { return threshold_; }
    double get_value() const { return value_; }
    const NodePtr& get_left() const { return left_; }
    const NodePtr& get_right() const { return right_; }

    // Setters
    void set_left(NodePtr left) { left_ = std::move(left); }
    void set_right(NodePtr right) { right_ = std::move(right); }

private:
    NodeType type_;                ///< 节点类型
    size_t feature_idx_;          ///< 分割特征索引
    double threshold_;            ///< 分割阈值
    double value_;               ///< 叶节点预测值
    size_t n_samples_;           ///< 样本数量
    double sum_;                 ///< 样本值之和
    double sum_squared_;         ///< 样本值平方和
    NodePtr left_;               ///< 左子节点
    NodePtr right_;              ///< 右子节点
};

} // namespace rf 
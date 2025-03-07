#include "core/tree_node.hpp"

namespace rf {

TreeNode::TreeNode(double value)
    : type_(NodeType::LEAF)
    , feature_idx_(0)
    , threshold_(0.0)
    , value_(value)
    , n_samples_(0)
    , sum_(0.0)
    , sum_squared_(0.0) {}

TreeNode::TreeNode(size_t feature_idx, double threshold)
    : type_(NodeType::INTERNAL)
    , feature_idx_(feature_idx)
    , threshold_(threshold)
    , value_(0.0)
    , n_samples_(0)
    , sum_(0.0)
    , sum_squared_(0.0) {}

double TreeNode::predict(const arma::vec& x) const {
    if (type_ == NodeType::LEAF) {
        return value_;
    }
    
    if (x(feature_idx_) <= threshold_) {
        return left_->predict(x);
    } else {
        return right_->predict(x);
    }
}

void TreeNode::update_stats(const arma::vec& x, double y) {
    n_samples_++;
    sum_ += y;
    sum_squared_ += y * y;
    
    if (type_ == NodeType::INTERNAL) {
        if (x(feature_idx_) <= threshold_) {
            left_->update_stats(x, y);
        } else {
            right_->update_stats(x, y);
        }
    } else {
        // 更新叶节点的预测值为均值
        value_ = sum_ / n_samples_;
    }
}

bool TreeNode::should_split(size_t min_samples_split, size_t max_depth, size_t current_depth) const {
    // 检查是否满足分裂条件
    if (type_ == NodeType::INTERNAL) {
        return false;  // 已经是内部节点
    }
    
    if (current_depth >= max_depth) {
        return false;  // 达到最大深度
    }
    
    if (n_samples_ < min_samples_split) {
        return false;  // 样本数不足
    }
    
    // 计算方差，检查是否值得分裂
    double mean = sum_ / n_samples_;
    double variance = (sum_squared_ / n_samples_) - (mean * mean);
    
    return variance > 1e-6;  // 方差足够大时分裂
}

} // namespace rf 
#include "core/adaptive_tree.hpp"
#include <algorithm>
#include <random>

namespace rf {

AdaptiveTree::AdaptiveTree(size_t max_depth, size_t min_samples_split, double lambda)
    : max_depth_(max_depth)
    , min_samples_split_(min_samples_split)
    , lambda_(lambda)
    , rng_(std::random_device{}()) {}

void AdaptiveTree::fit(const arma::mat& X, const arma::vec& y, size_t feature_subset_size) {
    // 初始化特征重要性向量
    feature_importance_ = std::vector<double>(X.n_cols, 0.0);
    
    // 确定特征子集大小
    if (feature_subset_size == 0) {
        feature_subset_size = static_cast<size_t>(std::sqrt(X.n_cols));
    }
    
    // 创建可用特征索引
    std::vector<size_t> available_features(X.n_cols);
    std::iota(available_features.begin(), available_features.end(), 0);
    
    // 构建树
    root_ = build_tree(X, y, 0, available_features);
}

arma::vec AdaptiveTree::predict(const arma::mat& X) const {
    arma::vec predictions(X.n_rows);
    
    #pragma omp parallel for
    for (size_t i = 0; i < X.n_rows; ++i) {
        predictions(i) = root_->predict(X.row(i).t());
    }
    
    return predictions;
}

void AdaptiveTree::update(const arma::mat& X_new, const arma::vec& y_new) {
    // 更新树的统计信息
    for (size_t i = 0; i < X_new.n_rows; ++i) {
        root_->update_stats(X_new.row(i).t(), y_new(i));
    }
}

arma::vec AdaptiveTree::get_feature_importance() const {
    return arma::vec(feature_importance_);
}

TreeNode::NodePtr AdaptiveTree::build_tree(const arma::mat& X, 
                                         const arma::vec& y,
                                         size_t depth,
                                         const std::vector<size_t>& available_features) {
    // 创建叶节点
    double mean_value = arma::mean(y);
    auto node = std::make_shared<TreeNode>(mean_value);
    
    // 更新节点统计信息
    for (size_t i = 0; i < X.n_rows; ++i) {
        node->update_stats(X.row(i).t(), y(i));
    }
    
    // 检查是否应该分裂
    if (!node->should_split(min_samples_split_, max_depth_, depth)) {
        return node;
    }
    
    // 寻找最佳分割点
    auto [best_feature, best_threshold] = find_best_split(X, y, available_features);
    
    // 如果找不到好的分割点，返回叶节点
    if (best_feature == size_t(-1)) {
        return node;
    }
    
    // 创建内部节点
    auto split_node = std::make_shared<TreeNode>(best_feature, best_threshold);
    
    // 分割数据
    arma::uvec left_indices = arma::find(X.col(best_feature) <= best_threshold);
    arma::uvec right_indices = arma::find(X.col(best_feature) > best_threshold);
    
    // 递归构建左右子树
    split_node->set_left(build_tree(X.rows(left_indices), y(left_indices), depth + 1, available_features));
    split_node->set_right(build_tree(X.rows(right_indices), y(right_indices), depth + 1, available_features));
    
    // 更新特征重要性
    double gain = calculate_split_gain(y(left_indices), y(right_indices));
    feature_importance_[best_feature] += gain;
    
    return split_node;
}

std::pair<size_t, double> AdaptiveTree::find_best_split(const arma::mat& X,
                                                       const arma::vec& y,
                                                       const std::vector<size_t>& available_features) {
    size_t best_feature = -1;
    double best_threshold = 0.0;
    double best_gain = -1.0;
    
    // 随机选择特征子集
    std::vector<size_t> feature_subset = available_features;
    std::shuffle(feature_subset.begin(), feature_subset.end(), rng_);
    
    // 对每个特征尝试找最佳分割点
    #pragma omp parallel for shared(best_feature, best_threshold, best_gain)
    for (size_t i = 0; i < feature_subset.size(); ++i) {
        size_t feature = feature_subset[i];
        arma::vec unique_values = arma::unique(X.col(feature));
        
        for (size_t j = 0; j < unique_values.n_elem - 1; ++j) {
            double threshold = (unique_values(j) + unique_values(j + 1)) / 2.0;
            
            arma::uvec left_indices = arma::find(X.col(feature) <= threshold);
            arma::uvec right_indices = arma::find(X.col(feature) > threshold);
            
            if (left_indices.n_elem < min_samples_split_ || right_indices.n_elem < min_samples_split_) {
                continue;
            }
            
            double gain = calculate_split_gain(y(left_indices), y(right_indices));
            
            #pragma omp critical
            {
                if (gain > best_gain) {
                    best_gain = gain;
                    best_feature = feature;
                    best_threshold = threshold;
                }
            }
        }
    }
    
    return {best_feature, best_threshold};
}

double AdaptiveTree::calculate_split_gain(const arma::vec& y_left, const arma::vec& y_right) const {
    double n = y_left.n_elem + y_right.n_elem;
    double n_left = y_left.n_elem;
    double n_right = y_right.n_elem;
    
    // 计算总体方差
    double total_var = arma::var(arma::join_vert(y_left, y_right));
    
    // 计算加权子集方差
    double left_var = arma::var(y_left);
    double right_var = arma::var(y_right);
    double weighted_var = (n_left * left_var + n_right * right_var) / n;
    
    // 返回方差减少量
    return total_var - weighted_var;
}

bool AdaptiveTree::should_prune(const TreeNode::NodePtr& node,
                              const arma::mat& X,
                              const arma::vec& y) const {
    if (node->get_type() == TreeNode::NodeType::LEAF) {
        return false;
    }
    
    // 计算不剪枝时的损失
    double unpruned_loss = 0.0;
    arma::vec predictions = predict(X);
    for (size_t i = 0; i < y.n_elem; ++i) {
        unpruned_loss += std::pow(y(i) - predictions(i), 2);
    }
    unpruned_loss = std::sqrt(unpruned_loss / y.n_elem);
    
    // 计算剪枝后的损失
    double pruned_loss = arma::mean(arma::square(y - arma::mean(y)));
    
    // 考虑复杂度惩罚
    return (pruned_loss - unpruned_loss) < lambda_;
}

} // namespace rf 
#include "core/distributed_forest.hpp"
#include <numeric>
#include <algorithm>
#include <fstream>

namespace rf {

DistributedForest::DistributedForest(size_t n_trees_total,
                                   size_t max_depth,
                                   size_t min_samples_split,
                                   double feature_ratio,
                                   double lambda)
    : n_trees_total_(n_trees_total)
    , max_depth_(max_depth)
    , min_samples_split_(min_samples_split)
    , feature_ratio_(feature_ratio)
    , lambda_(lambda) {
    init_mpi();
}

DistributedForest::~DistributedForest() {
    // MPI_Finalize() 将在main函数中调用
}

void DistributedForest::init_mpi() {
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size_);
    is_master_ = (rank_ == 0);
    
    // 计算每个进程的树的数量
    n_trees_local_ = n_trees_total_ / world_size_;
    if (static_cast<size_t>(rank_) < n_trees_total_ % world_size_) {
        n_trees_local_++;
    }
}

void DistributedForest::fit(const arma::mat& X, const arma::vec& y) {
    // 初始化本地树
    trees_.clear();
    trees_.reserve(n_trees_local_);
    
    // 计算特征子集大小
    size_t feature_subset_size = static_cast<size_t>(X.n_cols * feature_ratio_);
    
    // 并行训练本地树
    #pragma omp parallel for
    for (size_t i = 0; i < n_trees_local_; ++i) {
        auto tree = std::make_shared<AdaptiveTree>(max_depth_, min_samples_split_, lambda_);
        
        // 创建bootstrap样本
        arma::uvec indices = arma::randi<arma::uvec>(X.n_rows, arma::distr_param(0, X.n_rows - 1));
        arma::mat X_bootstrap = X.rows(indices);
        arma::vec y_bootstrap = y(indices);
        
        // 训练树
        tree->fit(X_bootstrap, y_bootstrap, feature_subset_size);
        
        #pragma omp critical
        {
            trees_.push_back(tree);
        }
    }
    
    // 同步特征重要性
    sync_feature_importance();
}

arma::vec DistributedForest::predict(const arma::mat& X) const {
    // 本地预测
    arma::mat local_predictions(X.n_rows, trees_.size());
    
    #pragma omp parallel for
    for (size_t i = 0; i < trees_.size(); ++i) {
        local_predictions.col(i) = trees_[i]->predict(X);
    }
    
    // 计算本地平均
    arma::vec local_mean = arma::mean(local_predictions, 1);
    
    // 收集所有进程的预测结果
    return gather_predictions(local_mean);
}

void DistributedForest::update(const arma::mat& X_new, const arma::vec& y_new) {
    // 并行更新所有本地树
    #pragma omp parallel for
    for (size_t i = 0; i < trees_.size(); ++i) {
        trees_[i]->update(X_new, y_new);
    }
    
    // 重新同步特征重要性
    sync_feature_importance();
}

arma::vec DistributedForest::get_feature_importance() const {
    return feature_importance_;
}

void DistributedForest::distribute_data(const arma::mat& X, const arma::vec& y) {
    // 广播数据维度
    size_t n_samples = X.n_rows;
    size_t n_features = X.n_cols;
    MPI_Bcast(&n_samples, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n_features, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    
    // 广播数据
    if (!is_master_) {
        const_cast<arma::mat&>(X).set_size(n_samples, n_features);
        const_cast<arma::vec&>(y).set_size(n_samples);
    }
    
    MPI_Bcast(const_cast<double*>(X.memptr()), n_samples * n_features, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(const_cast<double*>(y.memptr()), n_samples, MPI_DOUBLE, 0, MPI_COMM_WORLD);
}

arma::vec DistributedForest::gather_predictions(const arma::vec& local_preds) const {
    arma::vec global_preds;
    
    if (is_master_) {
        global_preds.zeros(local_preds.n_elem);
    }
    
    // 收集所有预测结果
    MPI_Reduce(const_cast<double*>(local_preds.memptr()),
               global_preds.memptr(),
               local_preds.n_elem,
               MPI_DOUBLE,
               MPI_SUM,
               0,
               MPI_COMM_WORLD);
    
    if (is_master_) {
        global_preds /= world_size_;
    }
    
    return global_preds;
}

void DistributedForest::sync_feature_importance() {
    // 收集所有树的特征重要性
    arma::vec local_importance(trees_[0]->get_feature_importance().n_elem, arma::fill::zeros);
    
    for (const auto& tree : trees_) {
        local_importance += tree->get_feature_importance();
    }
    
    // 在所有进程间同步
    if (is_master_) {
        feature_importance_.zeros(local_importance.n_elem);
    }
    
    MPI_Reduce(local_importance.memptr(),
               feature_importance_.memptr(),
               local_importance.n_elem,
               MPI_DOUBLE,
               MPI_SUM,
               0,
               MPI_COMM_WORLD);
    
    if (is_master_) {
        // 归一化特征重要性
        feature_importance_ /= arma::sum(feature_importance_);
    }
}

void DistributedForest::save_model(const std::string& filename) const {
    if (!is_master_) {
        return;  // 只在主进程中保存模型
    }

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法创建模型文件：" + filename);
    }

    // 保存超参数
    file.write(reinterpret_cast<const char*>(&n_trees_total_), sizeof(n_trees_total_));
    file.write(reinterpret_cast<const char*>(&max_depth_), sizeof(max_depth_));
    file.write(reinterpret_cast<const char*>(&min_samples_split_), sizeof(min_samples_split_));
    file.write(reinterpret_cast<const char*>(&feature_ratio_), sizeof(feature_ratio_));
    file.write(reinterpret_cast<const char*>(&lambda_), sizeof(lambda_));

    // 保存特征重要性
    size_t importance_size = feature_importance_.n_elem;
    file.write(reinterpret_cast<const char*>(&importance_size), sizeof(importance_size));
    file.write(reinterpret_cast<const char*>(feature_importance_.memptr()), 
               importance_size * sizeof(double));

    // 保存树的数量
    size_t n_trees = trees_.size();
    file.write(reinterpret_cast<const char*>(&n_trees), sizeof(n_trees));

    // 保存每棵树
    for (const auto& tree : trees_) {
        save_tree(file, tree->get_root());
    }
}

void DistributedForest::load_model(const std::string& filename) {
    if (!is_master_) {
        return;  // 只在主进程中加载模型
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("无法打开模型文件：" + filename);
    }

    // 加载超参数
    file.read(reinterpret_cast<char*>(&n_trees_total_), sizeof(n_trees_total_));
    file.read(reinterpret_cast<char*>(&max_depth_), sizeof(max_depth_));
    file.read(reinterpret_cast<char*>(&min_samples_split_), sizeof(min_samples_split_));
    file.read(reinterpret_cast<char*>(&feature_ratio_), sizeof(feature_ratio_));
    file.read(reinterpret_cast<char*>(&lambda_), sizeof(lambda_));

    // 加载特征重要性
    size_t importance_size;
    file.read(reinterpret_cast<char*>(&importance_size), sizeof(importance_size));
    feature_importance_.set_size(importance_size);
    file.read(reinterpret_cast<char*>(feature_importance_.memptr()), 
              importance_size * sizeof(double));

    // 加载树的数量
    size_t n_trees;
    file.read(reinterpret_cast<char*>(&n_trees), sizeof(n_trees));

    // 加载每棵树
    trees_.clear();
    trees_.reserve(n_trees);
    for (size_t i = 0; i < n_trees; ++i) {
        auto tree = std::make_shared<AdaptiveTree>(max_depth_, min_samples_split_, lambda_);
        tree->set_root(load_tree(file));
        trees_.push_back(tree);
    }

    // 广播模型到其他进程
    broadcast_model();
}

void DistributedForest::save_tree(std::ofstream& file, const TreeNode::NodePtr& node) const {
    if (!node) {
        bool is_null = true;
        file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));
        return;
    }

    bool is_null = false;
    file.write(reinterpret_cast<const char*>(&is_null), sizeof(bool));

    // 保存节点类型
    auto type = node->get_type();
    file.write(reinterpret_cast<const char*>(&type), sizeof(TreeNode::NodeType));

    if (type == TreeNode::NodeType::LEAF) {
        // 保存叶节点值
        double value = node->get_value();
        file.write(reinterpret_cast<const char*>(&value), sizeof(double));
    } else {
        // 保存内部节点信息
        size_t feature_idx = node->get_feature_idx();
        double threshold = node->get_threshold();
        file.write(reinterpret_cast<const char*>(&feature_idx), sizeof(size_t));
        file.write(reinterpret_cast<const char*>(&threshold), sizeof(double));

        // 递归保存子节点
        save_tree(file, node->get_left());
        save_tree(file, node->get_right());
    }
}

TreeNode::NodePtr DistributedForest::load_tree(std::ifstream& file) {
    bool is_null;
    file.read(reinterpret_cast<char*>(&is_null), sizeof(bool));
    if (is_null) {
        return nullptr;
    }

    TreeNode::NodeType type;
    file.read(reinterpret_cast<char*>(&type), sizeof(TreeNode::NodeType));

    if (type == TreeNode::NodeType::LEAF) {
        double value;
        file.read(reinterpret_cast<char*>(&value), sizeof(double));
        return std::make_shared<TreeNode>(value);
    } else {
        size_t feature_idx;
        double threshold;
        file.read(reinterpret_cast<char*>(&feature_idx), sizeof(size_t));
        file.read(reinterpret_cast<char*>(&threshold), sizeof(double));

        auto node = std::make_shared<TreeNode>(feature_idx, threshold);
        node->set_left(load_tree(file));
        node->set_right(load_tree(file));
        return node;
    }
}

void DistributedForest::broadcast_model() {
    // 广播超参数
    MPI_Bcast(&n_trees_total_, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_depth_, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&min_samples_split_, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&feature_ratio_, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&lambda_, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // 广播特征重要性
    if (!is_master_) {
        size_t importance_size;
        MPI_Bcast(&importance_size, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
        feature_importance_.set_size(importance_size);
    } else {
        size_t importance_size = feature_importance_.n_elem;
        MPI_Bcast(&importance_size, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    }
    MPI_Bcast(feature_importance_.memptr(), feature_importance_.n_elem, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // TODO: 实现树的MPI广播（如果需要）
}

} // namespace rf 
#pragma once

#include "adaptive_tree.hpp"
#include <mpi.h>
#include <vector>
#include <memory>
#include <fstream>

namespace rf {

/**
 * @brief 分布式随机森林类
 * 
 * 实现了基于MPI的分布式随机森林，支持多机训练和预测
 */
class DistributedForest {
public:
    /**
     * @brief 构造函数
     * @param n_trees_total 总树的数量
     * @param max_depth 最大树深度
     * @param min_samples_split 最小分裂样本数
     * @param feature_ratio 特征采样比例
     * @param lambda 复杂度惩罚系数
     */
    DistributedForest(size_t n_trees_total = 100,
                      size_t max_depth = 10,
                      size_t min_samples_split = 2,
                      double feature_ratio = 0.7,
                      double lambda = 0.1);

    /**
     * @brief 析构函数
     */
    ~DistributedForest();

    /**
     * @brief 训练模型
     * @param X 特征矩阵
     * @param y 目标值向量
     */
    void fit(const arma::mat& X, const arma::vec& y);

    /**
     * @brief 预测新样本
     * @param X 特征矩阵
     * @return 预测值向量
     */
    arma::vec predict(const arma::mat& X) const;

    /**
     * @brief 在线更新模型
     * @param X_new 新特征矩阵
     * @param y_new 新目标值向量
     */
    void update(const arma::mat& X_new, const arma::vec& y_new);

    /**
     * @brief 获取特征重要性分数
     * @return 特征重要性向量
     */
    arma::vec get_feature_importance() const;

    /**
     * @brief 检查是否为主进程
     * @return 是否为主进程
     */
    bool is_master() const { return is_master_; }

    /**
     * @brief 保存模型到文件
     * @param filename 文件名
     */
    void save_model(const std::string& filename) const;

    /**
     * @brief 从文件加载模型
     * @param filename 文件名
     */
    void load_model(const std::string& filename);

private:
    /**
     * @brief 初始化MPI环境
     */
    void init_mpi();

    /**
     * @brief 分发数据到各个进程
     * @param X 特征矩阵
     * @param y 目标值向量
     */
    void distribute_data(const arma::mat& X, const arma::vec& y);

    /**
     * @brief 收集所有进程的预测结果
     * @param local_preds 本地预测结果
     * @return 全局预测结果
     */
    arma::vec gather_predictions(const arma::vec& local_preds) const;

    /**
     * @brief 同步特征重要性
     */
    void sync_feature_importance();

    /**
     * @brief 保存单棵树到文件
     * @param file 输出文件流
     * @param node 当前节点
     */
    void save_tree(std::ofstream& file, const TreeNode::NodePtr& node) const;

    /**
     * @brief 从文件加载单棵树
     * @param file 输入文件流
     * @return 树的根节点
     */
    TreeNode::NodePtr load_tree(std::ifstream& file);

    /**
     * @brief 广播模型到所有进程
     */
    void broadcast_model();

private:
    int rank_;                  ///< MPI进程的rank
    int world_size_;           ///< MPI进程的总数
    bool is_master_;           ///< 是否是主进程
    size_t n_trees_total_;     ///< 总树的数量
    size_t n_trees_local_;     ///< 当前进程的树数量
    size_t max_depth_;         ///< 树的最大深度
    size_t min_samples_split_; ///< 分裂所需的最小样本数
    double feature_ratio_;     ///< 特征采样比例
    double lambda_;            ///< 正则化参数
    std::vector<std::shared_ptr<AdaptiveTree>> trees_; ///< 当前进程的树集合
    arma::vec feature_importance_;                      ///< 全局特征重要性
};

} // namespace rf 
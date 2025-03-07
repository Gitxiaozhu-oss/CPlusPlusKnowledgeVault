#pragma once

#include <armadillo>
#include <string>

namespace rf {

/**
 * @brief 数据加载器类
 */
class DataLoader {
public:
    /**
     * @brief 批次迭代器类
     */
    class BatchIterator {
    public:
        /**
         * @brief 构造函数
         * @param X 特征矩阵
         * @param y 目标值
         * @param batch_size 批次大小
         */
        BatchIterator(const arma::mat& X, const arma::vec& y, arma::uword batch_size);
        
        /**
         * @brief 检查是否还有下一个批次
         * @return 是否有下一个批次
         */
        bool has_next() const;
        
        /**
         * @brief 获取下一个批次
         * @return 是否成功获取下一个批次
         */
        bool next();

        /**
         * @brief 获取当前批次的特征矩阵
         * @return 当前批次的特征矩阵
         */
        const arma::mat& get_current_batch() const { return current_batch_; }

        /**
         * @brief 获取当前批次的标签
         * @return 当前批次的标签
         */
        const arma::vec& get_current_labels() const { return current_labels_; }
        
    private:
        const arma::mat& X_;              ///< 特征矩阵引用
        const arma::vec& y_;              ///< 目标值引用
        arma::uword batch_size_;          ///< 批次大小
        arma::uword current_pos_;         ///< 当前位置
        arma::mat current_batch_;         ///< 当前批次的特征矩阵
        arma::vec current_labels_;        ///< 当前批次的标签
    };

    /**
     * @brief 构造函数
     * @param batch_size 批次大小
     */
    explicit DataLoader(arma::uword batch_size = 32);

    /**
     * @brief 从CSV文件加载数据
     * @param filename 文件名
     * @param has_header 是否有表头
     * @return pair<特征矩阵, 目标值向量>
     */
    std::pair<arma::mat, arma::vec> load_csv(const std::string& filename, bool has_header = true);

    /**
     * @brief 使用内存映射加载大文件
     * @param filename 文件名
     * @param has_header 是否有表头
     * @return pair<特征矩阵, 目标值向量>
     */
    std::pair<arma::mat, arma::vec> load_large_csv(const std::string& filename, bool has_header = true);

    /**
     * @brief 获取批次迭代器
     * @param X 特征矩阵
     * @param y 目标值向量
     * @return BatchIterator对象
     */
    BatchIterator get_batch_iterator(const arma::mat& X, const arma::vec& y);

    /**
     * @brief 特征标准化
     * @param X 输入特征矩阵
     * @return 标准化后的特征矩阵
     */
    arma::mat standardize(const arma::mat& X);

    /**
     * @brief 特征归一化
     * @param X 输入特征矩阵
     * @return 归一化后的特征矩阵
     */
    arma::mat normalize(const arma::mat& X);

    /**
     * @brief 处理缺失值
     * @param X 输入特征矩阵
     * @param strategy 处理策略（"mean", "median", "mode"）
     * @return 处理后的特征矩阵
     */
    arma::mat handle_missing_values(const arma::mat& X, const std::string& strategy = "mean");

    /**
     * @brief 下载加州房价数据集
     * @param save_path 保存路径
     * @return pair<特征矩阵, 目标值向量>
     */
    static std::pair<arma::mat, arma::vec> load_california_housing(const std::string& save_path = "data/california_housing.csv");

private:
    arma::uword batch_size_;           ///< 批处理大小
    arma::vec mean_;              ///< 特征均值
    arma::vec std_;               ///< 特征标准差
    arma::vec min_;               ///< 特征最小值
    arma::vec max_;               ///< 特征最大值
}; 

} // namespace rf 
#include <iostream>
#include <string>
#include <mpi.h>
#include <random>
#include <chrono>
#include "core/distributed_forest.hpp"
#include "utils/data_loader.hpp"
#include "utils/metrics.hpp"

using namespace rf;
using namespace std;

/**
 * @brief 划分训练集和测试集
 * @param X 特征矩阵
 * @param y 目标值向量
 * @param test_ratio 测试集比例
 * @return pair<训练集, 测试集>，每个集合包含特征矩阵和目标值向量
 */
std::pair<std::pair<arma::mat, arma::vec>, std::pair<arma::mat, arma::vec>>
train_test_split(const arma::mat& X, const arma::vec& y, double test_ratio = 0.2) {
    size_t n_samples = X.n_rows;
    size_t n_test = static_cast<size_t>(n_samples * test_ratio);
    size_t n_train = n_samples - n_test;

    // 生成随机索引
    std::vector<size_t> indices(n_samples);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), 
                std::mt19937(std::chrono::system_clock::now().time_since_epoch().count()));

    // 划分训练集
    arma::mat X_train(n_train, X.n_cols);
    arma::vec y_train(n_train);
    for (size_t i = 0; i < n_train; ++i) {
        X_train.row(i) = X.row(indices[i]);
        y_train(i) = y(indices[i]);
    }

    // 划分测试集
    arma::mat X_test(n_test, X.n_cols);
    arma::vec y_test(n_test);
    for (size_t i = 0; i < n_test; ++i) {
        X_test.row(i) = X.row(indices[n_train + i]);
        y_test(i) = y(indices[n_train + i]);
    }

    return {{X_train, y_train}, {X_test, y_test}};
}

int main(int argc, char** argv) {
    // 初始化MPI
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    bool is_master = (rank == 0);
    
    try {
        arma::mat X;
        arma::vec y;
        
        // 只在主进程中加载数据
        if (is_master) {
            try {
                auto data = DataLoader::load_california_housing();
                X = std::move(data.first);
                y = std::move(data.second);
                
                if (X.n_rows == 0) {
                    throw std::runtime_error("数据集加载失败");
                }
                
                std::cout << "数据集加载成功！\n";
                std::cout << "样本数量: " << X.n_rows << "\n";
                std::cout << "特征数量: " << X.n_cols << "\n\n";
            } catch (const std::exception& e) {
                std::cerr << "数据加载错误：" << e.what() << std::endl;
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 1;
            }
        }
        
        // 广播数据维度
        arma::uword n_rows = X.n_rows;
        arma::uword n_cols = X.n_cols;
        MPI_Bcast(&n_rows, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
        MPI_Bcast(&n_cols, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
        
        // 在非主进程中分配内存
        if (!is_master) {
            X.set_size(n_rows, n_cols);
            y.set_size(n_rows);
        }
        
        // 广播数据
        MPI_Bcast(X.memptr(), X.n_elem, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(y.memptr(), y.n_elem, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // 划分训练集和测试集
        std::pair<arma::mat, arma::vec> train_data, test_data;
        if (is_master) {
            auto split_data = train_test_split(X, y, 0.2);
            train_data = std::move(split_data.first);
            test_data = std::move(split_data.second);
            std::cout << "数据集划分完成：\n";
            std::cout << "训练集大小: " << train_data.first.n_rows << "\n";
            std::cout << "测试集大小: " << test_data.first.n_rows << "\n\n";
        }

        // 广播训练集维度
        arma::uword n_train = train_data.first.n_rows;
        MPI_Bcast(&n_train, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);

        // 在非主进程中分配训练集内存
        if (!is_master) {
            train_data.first.set_size(n_train, n_cols);
            train_data.second.set_size(n_train);
        }

        // 广播训练集数据
        MPI_Bcast(train_data.first.memptr(), train_data.first.n_elem, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(train_data.second.memptr(), train_data.second.n_elem, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        
        // 创建分布式随机森林
        DistributedForest forest(100,  // 总树数
                               10,    // 最大深度
                               5,     // 最小分裂样本数
                               0.7,   // 特征比例
                               0.1);  // lambda
        
        // 训练模型
        if (is_master) {
            std::cout << "开始训练模型...\n";
        }
        forest.fit(train_data.first, train_data.second);
        
        // 在训练集上评估
        arma::vec y_train_pred = forest.predict(train_data.first);
        
        // 在测试集上评估
        arma::vec y_test_pred;
        if (is_master) {
            y_test_pred = forest.predict(test_data.first);
        }
        
        // 计算性能指标（只在主进程中进行）
        if (is_master) {
            Metrics metrics;
            auto train_metrics = metrics.calculate_regression_metrics(train_data.second, y_train_pred);
            auto test_metrics = metrics.calculate_regression_metrics(test_data.second, y_test_pred);
            
            std::cout << "训练完成！\n\n";
            std::cout << "训练集性能指标：\n";
            std::cout << "MSE: " << train_metrics.mse << "\n";
            std::cout << "RMSE: " << train_metrics.rmse << "\n";
            std::cout << "MAE: " << train_metrics.mae << "\n";
            std::cout << "R²: " << train_metrics.r2 << "\n";
            std::cout << "解释方差分数: " << train_metrics.explained_variance << "\n\n";
            
            std::cout << "测试集性能指标：\n";
            std::cout << "MSE: " << test_metrics.mse << "\n";
            std::cout << "RMSE: " << test_metrics.rmse << "\n";
            std::cout << "MAE: " << test_metrics.mae << "\n";
            std::cout << "R²: " << test_metrics.r2 << "\n";
            std::cout << "解释方差分数: " << test_metrics.explained_variance << "\n\n";
            
            // 输出特征重要性
            arma::vec importance = forest.get_feature_importance();
            std::cout << "特征重要性：\n";
            const char* feature_names[] = {
                "MedInc", "HouseAge", "AveRooms", "AveBedrms",
                "Population", "AveOccup", "Latitude", "Longitude"
            };
            for (size_t i = 0; i < importance.n_elem; ++i) {
                std::cout << feature_names[i] << ": " << importance(i) << "\n";
            }

            // 保存模型
            std::cout << "\n保存模型到 'random_forest_model.bin'...\n";
            try {
                forest.save_model("random_forest_model.bin");
                std::cout << "模型保存成功！\n";
            } catch (const std::exception& e) {
                std::cerr << "模型保存失败：" << e.what() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "错误：" << e.what() << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // 清理MPI
    MPI_Finalize();
    return 0;
} 
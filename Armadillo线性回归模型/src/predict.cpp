#include <iostream>
#include <string>
#include <mpi.h>
#include "core/distributed_forest.hpp"
#include "utils/data_loader.hpp"
#include "utils/metrics.hpp"

using namespace rf;
using namespace std;

int main(int argc, char** argv) {
    // 初始化MPI
    MPI_Init(&argc, &argv);
    
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    bool is_master = (rank == 0);
    
    try {
        // 加载数据
        arma::mat X;
        arma::vec y;
        
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
        
        // 创建分布式随机森林实例
        DistributedForest forest;
        
        // 加载模型
        if (is_master) {
            std::cout << "加载模型...\n";
            try {
                forest.load_model("random_forest_model.bin");
                std::cout << "模型加载成功！\n\n";
            } catch (const std::exception& e) {
                std::cerr << "模型加载失败：" << e.what() << "\n";
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 1;
            }
        }
        
        // 预测
        if (is_master) {
            arma::vec y_pred = forest.predict(X);
            
            // 计算性能指标
            Metrics metrics;
            auto regression_metrics = metrics.calculate_regression_metrics(y, y_pred);
            
            std::cout << "预测完成！性能指标：\n";
            std::cout << "MSE: " << regression_metrics.mse << "\n";
            std::cout << "RMSE: " << regression_metrics.rmse << "\n";
            std::cout << "MAE: " << regression_metrics.mae << "\n";
            std::cout << "R²: " << regression_metrics.r2 << "\n";
            std::cout << "解释方差分数: " << regression_metrics.explained_variance << "\n\n";
            
            // 输出一些预测示例
            std::cout << "预测示例（前5个样本）：\n";
            std::cout << "真实值\t\t预测值\t\t误差\n";
            for (size_t i = 0; i < 5 && i < y.n_elem; ++i) {
                std::cout << y(i) << "\t\t" 
                         << y_pred(i) << "\t\t"
                         << std::abs(y(i) - y_pred(i)) << "\n";
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
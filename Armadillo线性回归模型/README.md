# 分布式随机森林回归模型

这是一个基于C++实现的高性能分布式随机森林回归模型，使用MPI和OpenMP实现并行计算，支持大规模数据集的训练和预测。

## 主要特性

- **分布式训练**：使用MPI实现多机/多进程并行训练
- **并行计算**：使用OpenMP实现单机多线程并行
- **高性能矩阵运算**：基于Armadillo库实现高效的数值计算
- **自适应特征选择**：支持动态特征重要性评估
- **在线学习**：支持模型的增量更新
- **模型持久化**：支持模型的保存和加载
- **数据预处理**：支持特征标准化、归一化和缺失值处理

## 技术栈

- C++17
- MPI (消息传递接口)
- OpenMP (多线程并行)
- Armadillo (线性代数库)
- CMake (构建系统)
- vcpkg (包管理器)

## 项目结构

```
.
├── CMakeLists.txt          # CMake构建配置
├── README.md              # 项目说明文档
├── include/               # 头文件目录
│   ├── core/             # 核心功能模块
│   │   ├── adaptive_tree.hpp    # 自适应决策树
│   │   ├── distributed_forest.hpp # 分布式随机森林
│   │   └── tree_node.hpp        # 树节点结构
│   └── utils/            # 工具类
│       ├── data_loader.hpp      # 数据加载器
│       └── metrics.hpp          # 性能评估
├── src/                  # 源文件目录
│   ├── core/             # 核心实现
│   │   ├── adaptive_tree.cpp
│   │   ├── distributed_forest.cpp
│   │   └── tree_node.cpp
│   ├── utils/            # 工具实现
│   │   ├── data_loader.cpp
│   │   ├── data_loader_static.cpp
│   │   └── metrics.cpp
│   ├── main.cpp          # 训练程序入口
│   └── predict.cpp       # 预测程序入口
└── data/                 # 数据目录
    └── california_housing.csv  # 示例数据集
```

## 依赖要求

- C++17兼容的编译器
- CMake 3.15+
- MPI实现（如OpenMPI）
- OpenMP
- Armadillo 10.0.0+
- CURL
- vcpkg包管理器

## 安装步骤

1. 克隆项目并安装vcpkg：
```bash
git clone <repository_url>
cd <project_directory>
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
```

2. 安装依赖：
```bash
./vcpkg/vcpkg install armadillo:x64-osx
./vcpkg/vcpkg install openmpi:x64-osx
./vcpkg/vcpkg install curl:x64-osx
```

3. 构建项目：
```bash
mkdir build && cd build
cmake ..
make
```

## 使用说明

### 训练模型

```bash
# 使用4个进程训练模型
mpirun -np 4 ./random_forest
```

### 使用模型预测

```bash
# 使用4个进程进行预测
mpirun -np 4 ./predict
```

## 主要功能

### 1. 数据处理
- 支持CSV文件加载
- 特征标准化和归一化
- 缺失值处理
- 批量数据处理

### 2. 模型训练
- 自适应特征选择
- Bootstrap采样
- 并行树构建
- 特征重要性计算

### 3. 模型评估
- MSE（均方误差）
- RMSE（均方根误差）
- MAE（平均绝对误差）
- R²分数
- 解释方差分数

### 4. 模型部署
- 模型序列化
- 模型反序列化
- 分布式预测

## 性能优化

- 使用MPI实现数据并行
- 使用OpenMP实现任务并行
- 使用Armadillo优化矩阵运算
- 实现了高效的树结构序列化

## 示例

```cpp
// 创建随机森林实例
rf::DistributedForest forest(100,  // 树的数量
                            10,    // 最大深度
                            5,     // 最小分裂样本数
                            0.7,   // 特征采样比例
                            0.1);  // 正则化参数

// 训练模型
forest.fit(X_train, y_train);

// 预测
arma::vec y_pred = forest.predict(X_test);

// 保存模型
forest.save_model("random_forest_model.bin");

// 加载模型
forest.load_model("random_forest_model.bin");
```

## 贡献指南

欢迎提交问题和改进建议！请遵循以下步骤：

1. Fork本仓库
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件

## 模型效果评估

我们使用加州房价数据集对模型进行了测试，以下是模型在测试集上的表现：

### 数据集信息
```
数据集加载成功！
样本数量: 1000
特征数量: 8
```

### 性能指标
```
MSE: 6.94805e+09
RMSE: 83,355
MAE: 64,240.8
R²: 0.963641
解释方差分数: 0.963834
```

### 预测示例（前5个样本）
```
真实值          预测值          误差
220,873         174,867         46,006
1,067,840       1,083,120       15,280
336,968         221,296         115,672
154,817         266,936         112,119
441,973         405,034         36,939
```

### 模型效果分析

1. **整体性能评估**：
   - R²值达到0.963641，接近1，表明模型可以解释约96.36%的数据变异性
   - 解释方差分数为0.963834，与R²非常接近，说明模型预测的稳定性很好
   - 这两个指标都表明模型具有很强的预测能力

2. **误差分析**：
   - RMSE (均方根误差) = 83,355
   - MAE (平均绝对误差) = 64,240.8
   - 考虑到房价数据的规模（样本中有超过100万的房价），这些误差是可以接受的
   - MAE小于RMSE说明存在一些较大的异常预测值，但整体预测比较稳定

3. **预测效果分析**：
   - 对高价房产（约100万）的预测非常准确，误差仅1.4%
   - 对中等价位房产（约44万）的预测也比较准确，误差8.4%
   - 对部分低价位房产的预测误差较大
   - 说明模型在处理极端值或特殊情况时可能还需要改进

4. **改进建议**：
   - 增加决策树的数量（当前使用100棵树）
   - 调整特征采样比例（当前为0.7）
   - 改进低价位房产的预测准确度：
     - 添加更多相关特征
     - 优化数据预处理
     - 考虑使用分层采样策略

### 结论

该模型展现出了优秀的预测能力：
- 96%以上的R²和解释方差分数表明模型具有很强的预测能力
- 对大多数房价的预测误差在可接受范围内
- 特别是对中高价位房产的预测比较准确
- 模型已经可以用于实际应用，通过进一步优化可以获得更好的性能 
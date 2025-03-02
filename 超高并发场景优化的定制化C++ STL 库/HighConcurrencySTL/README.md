# HighConcurrencySTL

一个针对超高并发场景优化的定制化 C++ STL 库，专注于提供比标准 STL 容器更高性能的并发容器实现。

## 项目简介

HighConcurrencySTL 是一个现代 C++ 高性能并发容器库，专门为多线程高并发场景设计。通过使用无锁算法、细粒度锁定、分片技术和内存优化等方案，提供了一套可以在高并发环境下超越标准 STL 容器性能的替代方案。

### 主要特性

- **高性能并发容器**
  - `concurrent_vector`: 分段存储的并发向量
  - `concurrent_queue`: 无锁实现的并发队列
  - `concurrent_map`: 分片设计的并发哈希映射

- **核心优化技术**
  - 无锁数据结构设计
  - 细粒度锁定策略
  - 内存分配优化
  - 缓存友好的数据布局
  - 分片技术减少竞争

### 性能测试结果

#### Debug 模式性能数据

| 容器类型 | 线程数 | 操作速度 | 标准库速度 | 性能比例 |
|---------|--------|----------|------------|----------|
| concurrent_vector | 1 | 2.17M ops/s | 2.25M ops/s | 96.6% |
| std::vector + mutex | 1 | 2.25M ops/s | - | 100% |
| concurrent_vector | 4 | 1.45M ops/s | 3.17M ops/s | 45.7% |
| std::vector + mutex | 4 | 3.17M ops/s | - | 100% |
| concurrent_queue | 2 | 803K ops/s | 1.30M ops/s | 61.9% |
| std::queue + mutex | 2 | 1.30M ops/s | - | 100% |
| concurrent_queue | 4 | 1.07M ops/s | 1.04M ops/s | 102.8% |
| std::queue + mutex | 4 | 1.04M ops/s | - | 100% |
| concurrent_map | 1 | 1.74M ops/s | 1.33M ops/s | 131.0% |
| std::map + mutex | 1 | 1.33M ops/s | - | 100% |
| concurrent_map | 4 | 2.48M ops/s | 551K ops/s | 450.0% |
| std::map + mutex | 4 | 551K ops/s | - | 100% |

#### Release 模式性能数据（预期）

| 容器类型 | 线程数 | 操作速度 | 标准库速度 | 性能比例 |
|---------|--------|----------|------------|----------|
| concurrent_vector | 1 | 6.5M ops/s | 5.9M ops/s | 110% |
| std::vector + mutex | 1 | 5.9M ops/s | - | 100% |
| concurrent_vector | 4 | 4.3M ops/s | 7.2M ops/s | 60% |
| std::vector + mutex | 4 | 7.2M ops/s | - | 100% |
| concurrent_queue | 2 | 2.4M ops/s | 3.0M ops/s | 80% |
| std::queue + mutex | 2 | 3.0M ops/s | - | 100% |
| concurrent_queue | 4 | 3.2M ops/s | 2.1M ops/s | 150% |
| std::queue + mutex | 4 | 2.1M ops/s | - | 100% |
| concurrent_map | 1 | 5.2M ops/s | 3.7M ops/s | 140% |
| std::map + mutex | 1 | 3.7M ops/s | - | 100% |
| concurrent_map | 4 | 7.4M ops/s | 1.5M ops/s | 500% |
| std::map + mutex | 4 | 1.5M ops/s | - | 100% |

### 性能分析

1. **并发向量 (concurrent_vector)**
   - 单线程性能接近标准 vector
   - 多线程场景下性能略低，主要受分段锁开销影响
   - 适用于读多写少的场景

2. **并发队列 (concurrent_queue)**
   - 低并发时性能略低于互斥锁保护的标准队列
   - 高并发时表现优异，无锁设计优势显现
   - 特别适合生产者-消费者模式

3. **并发映射 (concurrent_map)**
   - 单线程性能超过标准 map
   - 多线程场景下性能优势显著
   - 分片设计有效减少了锁竞争

## 技术栈

- **编程语言**: C++20
- **构建系统**: CMake 3.15+
- **依赖项**:
  - Google Test (单元测试)
  - Google Benchmark (性能测试)
  - Intel TBB (线程管理)

## 优化技术详解

### 1. 并发向量优化
- 分段存储设计，减少锁竞争
- 使用共享互斥锁(shared_mutex)实现读写分离
- 自定义内存分配器，减少内存碎片
- 缓存对齐的数据结构

### 2. 并发队列优化
- 无锁算法实现，消除互斥开销
- 原子操作保证线程安全
- 使用内存屏障确保内存序
- 支持多生产者多消费者模式

### 3. 并发映射优化
- 分片设计减少锁粒度
- 高效的哈希算法
- 细粒度锁定策略
- 读写分离的并发控制

## 使用指南

### 1. 环境要求
```bash
# 编译器要求
- GCC 10+ 或
- Clang 10+ 或
- MSVC 2019+

# 必需工具
- CMake 3.15+
- Git
```

### 2. 构建项目
```bash
# 克隆项目
git clone https://github.com/Gitxiaozhu-oss/CPlusPlusKnowledgeVault.git
cd CPlusPlusKnowledgeVault

# 创建构建目录
mkdir build && cd build

# 配置项目（Debug模式）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 配置项目（Release模式）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译项目
cmake --build .
```

### 3. 使用示例

```cpp
#include <hcstl/concurrent_vector.hpp>
#include <hcstl/concurrent_queue.hpp>
#include <hcstl/concurrent_map.hpp>

// 使用并发向量
hcstl::concurrent_vector<int> vec;
vec.push_back(42);  // 线程安全的插入

// 使用并发队列
hcstl::concurrent_queue<int> queue;
queue.push(1);      // 线程安全的入队
int value;
queue.try_pop(value); // 线程安全的出队

// 使用并发映射
hcstl::concurrent_map<int, std::string> map;
map.insert(1, "one"); // 线程安全的插入
auto value = map.find(1); // 线程安全的查找
```
## 项目运行指南

### 1. 测试程序说明

项目包含三种类型的测试程序：

#### 单元测试 (container_tests)
```bash
# 运行单元测试
./tests/container_tests
```

单元测试程序测试了以下功能：
- **并发向量测试**
  - 基本操作测试：push_back、at、size、empty
  - 并发插入测试：4个线程同时插入1000个元素
  - 边界条件测试：空容器、大量数据

- **并发队列测试**
  - 基本操作测试：push、try_pop、size、empty
  - 生产者-消费者测试：2生产者2消费者
  - 压力测试：高频率push/pop操作

- **并发映射测试**
  - 基本操作测试：insert、find、erase
  - 并发访问测试：多线程同时插入和查找
  - 数据一致性测试：检查所有操作结果

#### 性能基准测试 (container_benchmarks)
```bash
# 运行性能测试（Debug模式）
./benchmarks/container_benchmarks

# 运行性能测试（Release模式）
cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .
./benchmarks/container_benchmarks
```

基准测试程序包含以下测试：
- **向量性能测试**
  - BM_ConcurrentVectorPushBack：测试并发push_back性能
  - BM_StdVectorPushBack：对比标准vector性能
  - 线程数范围：1-4线程
  - 每线程操作数：100次插入

- **队列性能测试**
  - BM_ConcurrentQueuePushPop：测试并发队列性能
  - BM_StdQueuePushPop：对比标准queue性能
  - 线程数范围：2-4线程（各半生产者消费者）
  - 每生产者操作数：100次push/pop

- **映射性能测试**
  - BM_ConcurrentMapInsertFind：测试并发映射性能
  - BM_StdMapInsertFind：对比标准map性能
  - 线程数范围：1-4线程
  - 每线程操作数：50次insert + 50次find

#### 示例程序 (container_example)
```bash
# 运行示例程序
./examples/container_example
```

示例程序演示了：
- **并发向量示例**
  - 4个线程并发插入10000个元素
  - 测量总执行时间
  - 验证结果正确性

- **并发队列示例**
  - 2生产者并发生产10000个元素
  - 2消费者并发消费所有元素
  - 测量生产消费总时间

- **并发映射示例**
  - 4个线程并发插入和查找1000个键值对
  - 测量总执行时间
  - 验证数据一致性

### 2. 完整运行流程

```bash
# 1. 创建并进入构建目录
mkdir build && cd build

# 2. 配置项目（Debug模式）
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 3. 编译所有目标
cmake --build .

# 4. 运行单元测试
./tests/container_tests

# 5. 运行性能测试
./benchmarks/container_benchmarks

# 6. 运行示例程序
./examples/container_example

# 7. 切换到Release模式重新测试（可选）
cd ..
mkdir build_release && cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
./benchmarks/container_benchmarks
```

### 3. 测试结果分析

- **单元测试输出**：显示每个测试用例的通过/失败状态
- **基准测试输出**：显示详细的性能指标，包括
  - 每秒操作数（ops/s）
  - CPU时间和实际时间
  - 迭代次数
  - 用户计数器（如处理的项目数）
- **示例程序输出**：显示实际应用场景下的性能数据

## 最佳实践

1. **选择合适的容器**
   - 读多写少：使用 concurrent_vector
   - 生产者-消费者模式：使用 concurrent_queue
   - 需要键值对存储：使用 concurrent_map

2. **性能优化建议**
   - 在 Release 模式下编译以获得最佳性能
   - 根据实际场景调整分片大小
   - 合理设置线程数量，避免过度竞争
   - 预分配容器容量，减少扩容开销

3. **注意事项**
   - 避免频繁的容器大小调整
   - 合理使用内存序（memory order）
   - 注意避免死锁和活锁
   - 定期监控性能指标

## 贡献指南

欢迎提交 Pull Request 或 Issue。在提交代码前，请确保：

1. 通过所有单元测试
2. 编写了相应的测试用例
3. 遵循项目的代码规范
4. 更新了相关文档

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

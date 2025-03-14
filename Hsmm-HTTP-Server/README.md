# HSMM HTTP Server

基于现代C++的高安全内存管理HTTP服务器

## 项目介绍

HSMM HTTP Server是一个基于现代C++20开发的高性能HTTP服务器，专注于内存安全和高并发处理。该项目采用了最新的C++特性和最佳实践，提供了一个安全、高效、可扩展的HTTP服务器框架。

### 核心特性

- **现代C++20特性**
  - 使用`std::span`进行安全的内存访问
  - 使用`std::string_view`实现零拷贝字符串处理
  - 支持协程和异步编程

- **内存安全**
  - 多态内存资源（PMR）支持
  - RAII资源管理
  - 智能指针自动内存管理
  - 边界检查和缓冲区保护

- **高性能设计**
  - 基于Boost.Asio的异步IO
  - 线程池并发处理
  - 零拷贝数据传输
  - 无锁队列日志系统

- **可靠性保证**
  - 全面的错误处理
  - 异常安全
  - 资源自动回收
  - 超时保护机制

## 系统架构

### 核心组件

1. **Server类** (`include/server/Server.hpp`)
   - 服务器核心类，管理连接和线程池
   - 处理客户端连接请求
   - 维护工作线程池
   - 实现优雅关闭机制

2. **Connection类** (`include/server/Connection.hpp`)
   - 管理单个HTTP连接
   - 处理请求读取和响应发送
   - 实现HTTP协议解析
   - 支持keep-alive连接

3. **HttpParser类** (`include/http/HttpParser.hpp`)
   - HTTP请求解析器
   - 支持请求头和请求体解析
   - 处理HTTP方法、URI和头部字段
   - 提供响应生成功能

4. **SafeBuffer类** (`include/utils/SafeBuffer.hpp`)
   - 安全的缓冲区实现
   - 提供边界检查
   - 支持PMR内存分配
   - 自动内存管理

5. **Logger类** (`include/utils/Logger.hpp`)
   - 线程安全的日志系统
   - 支持多级别日志
   - 文件和控制台输出
   - 时间戳和格式化输出

## 构建要求

- C++20兼容的编译器（GCC 10+, Clang 10+, MSVC 2019+）
- CMake 3.15+
- Boost 1.74+

## 构建步骤

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译项目
cmake --build .
```

## 运行服务器

```bash
# 使用默认配置运行（地址：0.0.0.0，端口：8080）
./bin/hsmmserver

# 指定地址和端口运行
./bin/hsmmserver 127.0.0.1 8000

# 指定地址、端口和线程数
./bin/hsmmserver 127.0.0.1 8000 4
```

## 性能测试

### 压力测试工具

项目包含了一个专门的压力测试工具 `stress_test`，用于评估服务器的性能和稳定性。

#### 使用方法

```bash
./bin/stress_test <host> <port> <total_requests> <concurrent_connections>

# 示例：发送1000个请求，使用10个并发连接
./bin/stress_test 127.0.0.1 8080 1000 10

# 大规模测试：发送10000个请求，使用20个并发连接
./bin/stress_test 127.0.0.1 8080 10000 20
```

#### 参数说明
- `host`: 服务器地址
- `port`: 服务器端口
- `total_requests`: 总请求数
- `concurrent_connections`: 并发连接数

#### 测试指标
- 总请求数和成功率
- 每秒查询数（QPS）
- 平均响应延迟
- 错误请求数量
- 总测试时间

### 测试结果示例

在本地测试环境下（MacBook Pro），服务器表现：

1. 小规模测试（1000请求，10并发）：
   - QPS: ~5300
   - 平均延迟: ~1.25ms
   - 成功率: 100%

2. 中等规模测试（10000请求，20并发）：
   - QPS: ~8000
   - 平均延迟: ~1.96ms
   - 成功率: 100%

## API接口

### HTTP请求处理

服务器支持标准的HTTP/1.1协议，当前实现的接口包括：

1. **GET 请求**
   - 路径: `/`
   - 响应: 纯文本消息
   - 示例响应:
     ```
     HTTP/1.1 200 OK
     Content-Length: 28
     Server: HSMM-HTTP-Server
     Content-Type: text/plain

     Hello from HSMM HTTP Server!
     ```

2. **错误处理**
   - 400 Bad Request: 请求格式错误
   - 404 Not Found: 资源不存在
   - 500 Internal Server Error: 服务器内部错误

## 安全特性

- 使用`std::unique_ptr`和`std::shared_ptr`进行自动内存管理
- 使用`std::span`进行安全的缓冲区访问
- 使用`std::string_view`实现零拷贝字符串处理
- PMR内存池减少内存碎片
- RAII确保资源正确释放
- 支持AddressSanitizer进行运行时检测

## 许可证

MIT License 
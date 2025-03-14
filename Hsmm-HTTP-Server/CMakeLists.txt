cmake_minimum_required(VERSION 3.15)
project(HsmmHttpServer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 启用编译警告
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 启用Address Sanitizer (仅在Debug模式)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# 查找Boost库
find_package(Boost REQUIRED COMPONENTS system)

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 源文件
set(SERVER_SOURCES
    src/main.cpp
    src/server/Server.cpp
    src/server/Connection.cpp
    src/http/HttpParser.cpp
)

# 主可执行文件
add_executable(hsmmserver ${SERVER_SOURCES})

# 压力测试程序
add_executable(stress_test src/test/stress_test.cpp)

# 链接Boost库
target_link_libraries(hsmmserver PRIVATE
    Boost::system
    pthread
)

target_link_libraries(stress_test PRIVATE
    Boost::system
    pthread
)

# 包含目录
target_include_directories(hsmmserver PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
)

target_include_directories(stress_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
) 
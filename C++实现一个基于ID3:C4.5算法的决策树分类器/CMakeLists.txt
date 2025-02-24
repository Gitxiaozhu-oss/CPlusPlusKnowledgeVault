cmake_minimum_required(VERSION 3.10)
project(decision_tree_iris)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(decision_tree
    src/main.cpp
    src/DecisionTree.cpp
)

# 添加头文件目录
target_include_directories(decision_tree PRIVATE src) 
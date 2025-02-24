cmake_minimum_required(VERSION 3.10)
project(StockPricePredictor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找必要的包
find_package(Boost REQUIRED COMPONENTS date_time)
find_package(Eigen3 REQUIRED)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${EIGEN3_INCLUDE_DIR})

# 添加源文件
file(GLOB SOURCES "src/*.cpp")

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    Eigen3::Eigen
) 
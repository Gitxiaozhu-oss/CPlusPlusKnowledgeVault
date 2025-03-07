cmake_minimum_required(VERSION 3.15)
project(advanced_random_forest CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译选项
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 设置编译优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# 设置MPI编译器
set(MPI_CXX_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-osx/tools/openmpi/bin/mpicxx")
set(MPI_C_COMPILER "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-osx/tools/openmpi/bin/mpicc")

# 设置OpenMP配置
if(APPLE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/usr/local/Homebrew/Cellar/libomp/20.1.0/include")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "/usr/local/Homebrew/Cellar/libomp/20.1.0/lib/libomp.dylib")
    endif()
endif()

# 设置vcpkg工具链
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# 设置Armadillo路径
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/x64-osx")

# 查找依赖包
find_package(Armadillo REQUIRED)
find_package(OpenMP REQUIRED)
find_package(MPI REQUIRED)
find_package(CURL REQUIRED)

# 添加头文件路径
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${ARMADILLO_INCLUDE_DIRS})
include_directories(${MPI_INCLUDE_PATH})
include_directories(${CURL_INCLUDE_DIRS})

# 添加源文件
set(SOURCES
    src/core/adaptive_tree.cpp
    src/core/distributed_forest.cpp
    src/core/tree_node.cpp
    src/utils/data_loader.cpp
    src/utils/data_loader_static.cpp
    src/utils/metrics.cpp
)

# 创建训练可执行文件
add_executable(random_forest ${SOURCES} src/main.cpp)

# 创建预测可执行文件
add_executable(predict ${SOURCES} src/predict.cpp)

# 链接库
foreach(target random_forest predict)
    target_link_libraries(${target}
        PRIVATE
        ${ARMADILLO_LIBRARIES}
        OpenMP::OpenMP_CXX
        MPI::MPI_CXX
        CURL::libcurl
    )

    # 设置编译选项
    target_compile_options(${target}
        PRIVATE
        -Wall
        -Wextra
        -O3
    )
endforeach()

# 如果是macOS，添加OpenMP库路径
if(APPLE)
    foreach(target random_forest predict)
        target_link_directories(${target} PRIVATE /usr/local/Homebrew/Cellar/libomp/20.1.0/lib)
        target_link_libraries(${target} PRIVATE omp)
    endforeach()
endif() 
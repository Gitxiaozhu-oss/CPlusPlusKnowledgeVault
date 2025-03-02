cmake_minimum_required(VERSION 3.15)
project(HighConcurrencySTL VERSION 0.1.0 LANGUAGES CXX)

# 设置 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# 添加第三方依赖
include(FetchContent)

# 添加 Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# 添加 Google Benchmark
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(benchmark)

# 添加 TBB
find_package(TBB REQUIRED)

# 添加头文件目录
include_directories(${PROJECT_SOURCE_DIR}/include)

# 添加库目标
add_library(hcstl INTERFACE)
target_include_directories(hcstl INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# 添加子目录
add_subdirectory(tests)
add_subdirectory(benchmarks)
add_subdirectory(examples)

# 安装配置
install(TARGETS hcstl
    EXPORT hcstl-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/hcstl
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# 导出配置
install(EXPORT hcstl-targets
    FILE hcstl-targets.cmake
    NAMESPACE hcstl::
    DESTINATION lib/cmake/hcstl
)

# 生成并安装版本文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/hcstl-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 配置并安装包配置文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/hcstl-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/hcstl-config.cmake"
    INSTALL_DESTINATION lib/cmake/hcstl
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/hcstl-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/hcstl-config-version.cmake"
    DESTINATION lib/cmake/hcstl
) 
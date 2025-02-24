cmake_minimum_required(VERSION 3.10)
project(MovieRecommender)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找SQLite3
find_package(SQLite3 REQUIRED)

# 添加源文件
set(SOURCES
    main.cpp
    MovieRecommender.cpp
    Database.cpp
    SVD.cpp
)

# 添加头文件
set(HEADERS
    MovieRecommender.h
    Database.h
    SVD.h
)

# 创建可执行文件
add_executable(movie_recommender ${SOURCES} ${HEADERS})

# 链接SQLite3
target_link_libraries(movie_recommender PRIVATE SQLite::SQLite3)

# 添加编译选项
if(MSVC)
    target_compile_options(movie_recommender PRIVATE /W4)
else()
    target_compile_options(movie_recommender PRIVATE -Wall -Wextra -Wpedantic)
endif() 
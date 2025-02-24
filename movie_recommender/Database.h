#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <vector>
#include <sqlite3.h>
#include <memory>

struct Rating {
    int userId;
    int movieId;
    double rating;
    Rating(int u, int m, double r) : userId(u), movieId(m), rating(r) {}
};

class Database {
public:
    // 构造函数
    explicit Database(const std::string& dbPath);
    ~Database();

    // 初始化数据库
    void initialize();

    // 添加评分
    void addRating(int userId, int movieId, double rating);

    // 获取所有评分
    std::vector<Rating> getAllRatings();

    // 获取用户的所有评分
    std::vector<Rating> getUserRatings(int userId);

    // 获取电影的所有评分
    std::vector<Rating> getMovieRatings(int movieId);

private:
    sqlite3* db_;
    std::string dbPath_;

    // 执行SQL语句
    void executeQuery(const std::string& sql);
    
    // 创建表
    void createTables();
};

#endif // DATABASE_H 
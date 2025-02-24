#include "Database.h"
#include <stdexcept>
#include <iostream>

Database::Database(const std::string& dbPath) : dbPath_(dbPath), db_(nullptr) {
    initialize();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

void Database::initialize() {
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc) {
        throw std::runtime_error("无法打开数据库: " + std::string(sqlite3_errmsg(db_)));
    }
    createTables();
}

void Database::createTables() {
    const char* sql = 
        "CREATE TABLE IF NOT EXISTS ratings ("
        "user_id INTEGER NOT NULL,"
        "movie_id INTEGER NOT NULL,"
        "rating REAL NOT NULL,"
        "PRIMARY KEY (user_id, movie_id));";
    
    executeQuery(sql);
}

void Database::executeQuery(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::string error = errMsg;
        sqlite3_free(errMsg);
        throw std::runtime_error("SQL错误: " + error);
    }
}

void Database::addRating(int userId, int movieId, double rating) {
    std::string sql = "INSERT OR REPLACE INTO ratings (user_id, movie_id, rating) "
                      "VALUES (" + std::to_string(userId) + ", " +
                      std::to_string(movieId) + ", " +
                      std::to_string(rating) + ");";
    executeQuery(sql);
}

std::vector<Rating> Database::getAllRatings() {
    std::vector<Rating> ratings;
    const char* sql = "SELECT user_id, movie_id, rating FROM ratings;";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        throw std::runtime_error("准备SQL语句失败: " + std::string(sqlite3_errmsg(db_)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        int movieId = sqlite3_column_int(stmt, 1);
        double rating = sqlite3_column_double(stmt, 2);
        ratings.emplace_back(userId, movieId, rating);
    }
    
    sqlite3_finalize(stmt);
    return ratings;
}

std::vector<Rating> Database::getUserRatings(int userId) {
    std::vector<Rating> ratings;
    std::string sql = "SELECT user_id, movie_id, rating FROM ratings WHERE user_id = " + 
                     std::to_string(userId) + ";";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        throw std::runtime_error("准备SQL语句失败: " + std::string(sqlite3_errmsg(db_)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int movieId = sqlite3_column_int(stmt, 1);
        double rating = sqlite3_column_double(stmt, 2);
        ratings.emplace_back(userId, movieId, rating);
    }
    
    sqlite3_finalize(stmt);
    return ratings;
}

std::vector<Rating> Database::getMovieRatings(int movieId) {
    std::vector<Rating> ratings;
    std::string sql = "SELECT user_id, movie_id, rating FROM ratings WHERE movie_id = " + 
                     std::to_string(movieId) + ";";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        throw std::runtime_error("准备SQL语句失败: " + std::string(sqlite3_errmsg(db_)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int userId = sqlite3_column_int(stmt, 0);
        double rating = sqlite3_column_double(stmt, 2);
        ratings.emplace_back(userId, movieId, rating);
    }
    
    sqlite3_finalize(stmt);
    return ratings;
} 
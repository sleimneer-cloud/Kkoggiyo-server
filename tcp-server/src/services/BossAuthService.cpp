#include "services/BossAuthService.hpp"
#include <iostream>
#include <mysql/mysql.h>
#include "DBConnectionPool.h"

bool BossAuthService::processLogin(int fd, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId = payload["userId"];
    std::string inputPw = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string query =
        "SELECT password FROM members WHERE user_id = '" + userId +
        "' AND role = 2";

    if (mysql_query(conn, query.c_str()))
    {
        std::cerr << "[BossAuthService] " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res)
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    bool success = false;
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && std::string(row[0]) == inputPw)
    {
        std::cout << "[BossAuthService] 사장님 로그인 성공: " << userId << "\n";
        success = true;
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

bool BossAuthService::processRegister(const nlohmann::json &payload)
{
    // TODO: 사장님 가입 구현
    return false;
}
#include "services/CustomerAuthService.hpp"
#include <iostream>
#include <mysql/mysql.h> // MariaDB/MySQL C API 헤더
#include "DBConnectionPool.h"

// ---------------------------------------------------------
// 1. 회원가입 로직 (INSERT)
// ---------------------------------------------------------
bool CustomerAuthService::processRegister(const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password") ||
        !payload.contains("userName") || !payload.contains("phone"))
    {
        return false;
    }

    std::string userId = payload["userId"];
    std::string password = payload["password"];
    std::string userName = payload["userName"];
    std::string phone = payload["phone"];
    std::string address = payload.value("address", "");

    MYSQL *conn = DBConnectionPool::getInstance().getConnection(); // DB 커넥션 풀에서 연결 하나 빌려오기
    if (!conn)                                                     // 연결 실패 시
    {
        return false;
    }

    std::string query = "INSERT INTO members (user_id, password, user_name, role, phone, address) VALUES ('" + userId + "', '" + password + "', '" + userName + "', 1, '" + phone + "', '" + address + "')";

    if (mysql_query(conn, query.c_str())) // 쿼리 실행 실패 시
    {
        std::cerr << "[Register Error] " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    std::cout << "[CustomerAuthService] 회원가입 완료: " << userId << "\n";
    DBConnectionPool::getInstance().releaseConnection(conn); // 사용한 연결 반납
    return true;
}

// ---------------------------------------------------------
// 2. 로그인 로직 (SELECT)
// ---------------------------------------------------------
bool CustomerAuthService::processLogin(int fd, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId = payload["userId"];
    std::string inputPassword = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection(); // DB 커넥션 풀에서 연결 하나 빌려오기
    if (!conn)
        return false;

    std::string query = "SELECT password FROM members WHERE user_id = '" + userId + "' AND role = 1";

    if (mysql_query(conn, query.c_str())) // 쿼리 실행 실패 시
    {
        std::cerr << "[Login Error] " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    MYSQL_RES *res = mysql_store_result(conn); // 결과 저장
    if (res == NULL)                           // 결과 저장 실패 시
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    bool isSuccess = false;
    MYSQL_ROW row = mysql_fetch_row(res); // 결과에서 한 행 가져오기 (user_id는 유니크하므로 최대 한 행만 나옴)

    if (row != NULL) // 해당 userId가 DB에 존재한다면
    {
        std::string dbPassword = row[0];
        if (dbPassword == inputPassword)
        {
            std::cout << "[CustomerAuthService] 로그인 성공: " << userId << "\n";
            isSuccess = true;
        }
    }

    mysql_free_result(res);                                  // 결과 메모리 해제
    DBConnectionPool::getInstance().releaseConnection(conn); // 사용한 연결 반납
    return isSuccess;
}
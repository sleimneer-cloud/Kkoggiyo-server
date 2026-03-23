#include "services/CustomerAuthService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp" // <--- DBHelper 헤더 추가
#include <iostream>

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

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    // 변경된 부분: Prepared Statement용 쿼리와 파라미터 벡터 사용
    std::string query = "INSERT INTO member (login_id, password, name, role, phone) VALUES (?, ?, ?, 1, ?)";
    std::vector<std::string> params = {userId, password, userName, phone};

    // DBHelper를 통해 쿼리 실행
    if (!DBHelper::executeUpdate(conn, query, params))
    {
        std::cerr << "[CustomerAuthService] 회원가입 실패: " << userId << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    std::cout << "[CustomerAuthService] 회원가입 완료: " << userId << "\n";
    DBConnectionPool::getInstance().releaseConnection(conn);
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

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    // 변경된 부분: Prepared Statement용 쿼리와 파라미터 벡터 사용
    std::string query = "SELECT password FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};
    std::string dbPassword;

    bool isSuccess = false;

    // DBHelper가 ? 자리에 userId를 넣고 결과를 dbPassword에 담아줌
    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword))
    {
        if (dbPassword == inputPassword)
        {
            std::cout << "[CustomerAuthService] 로그인 성공: " << userId << "\n";
            isSuccess = true;
        }
        else
        {
            std::cout << "[CustomerAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
        }
    }
    else
    {
        std::cout << "[CustomerAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return isSuccess;
}
#include "services/BossAuthService.hpp"
#include <iostream>
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <crypt.h> // crypt() — bcrypt 검증 (링크: -lcrypt)

// ---------------------------------------------------------
// bcrypt 검증 유틸 (이 파일 내부에서만 사용)
// ---------------------------------------------------------
static bool bcryptVerify(const std::string &password, const std::string &hash)
{
    if (hash.empty()) return false;
    const char *result = crypt(password.c_str(), hash.c_str());
    return result && (hash == std::string(result));
}

// ---------------------------------------------------------
// 로그인 로직
// ---------------------------------------------------------
bool BossAuthService::processLogin(int fd, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId  = payload["userId"];
    std::string inputPw = payload["password"]; // 클라이언트 SHA256 해싱값

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string query = "SELECT password FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> params = {userId};
    std::string dbPassword; // DB에 저장된 bcrypt 해시

    bool success = false;

    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword))
    {
        // ✅ 수정: == 단순 비교 → bcryptVerify로 검증
        if (bcryptVerify(inputPw, dbPassword))
        {
            std::cout << "[BossAuthService] 사장님 로그인 성공: " << userId << "\n";
            success = true;
        }
        else
        {
            std::cout << "[BossAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
        }
    }
    else
    {
        std::cout << "[BossAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

// ---------------------------------------------------------
// 회원가입 로직
// ---------------------------------------------------------
bool BossAuthService::processRegister(const nlohmann::json &payload)
{
    // TODO: 사장님 가입 구현
    // CustomerAuthService::processRegister와 동일한 패턴으로 bcryptHash() 적용할 것
    return false;
}

#include "services/AdminAuthService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <crypt.h>        // crypt() — bcrypt 지원 (링크: -lcrypt)
#include <openssl/rand.h> // RAND_bytes() — 암호학적 난수 (링크: -lssl -lcrypto)

// ---------------------------------------------------------
// bcrypt 검증 유틸 (이 파일 내부에서만 사용)
// ---------------------------------------------------------
static bool bcryptVerify(const std::string &password, const std::string &hash)
{
    if (hash.empty())
        return false;
    const char *result = crypt(password.c_str(), hash.c_str());
    return result && (hash == std::string(result));
}
// 암호화 함수

static std::string generateBcryptSalt() // bcrypt 솔트 생성 (22자 + 버전/비용 정보 포함)
{
    static const char b64chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";

    unsigned char rnd[22];
    RAND_bytes(rnd, sizeof(rnd));

    std::string salt = "$2b$12$";
    for (int i = 0; i < 22; ++i)
        salt += b64chars[rnd[i] % 64];

    return salt;
}

static std::string bcryptHash(const std::string &password) // bcrypt 해시 생성
{
    std::string salt = generateBcryptSalt();
    const char *hashed = crypt(password.c_str(), salt.c_str());
    return hashed ? std::string(hashed) : "";
}
// ---------------------------------------------------------
// 로그인 로직
// ---------------------------------------------------------
bool AdminAuthService::processLogin(int fd, const nlohmann::json &payload, std::string &outMsg)
{
    if (!payload.contains("userId") || !payload.contains("password")) {
        outMsg = "로그인 정보가 누락되었습니다.";
        return false;
    }

    std::string userId = payload["userId"];
    std::string inputPw = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // is_active 확인을 위해 쿼리에서 분리 (모든 계정을 조회 후 비교)
    std::string query = "SELECT password, is_active FROM admin_log WHERE email = ?";
    std::vector<std::string> params = {userId};

    bool success = false;
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(conn, query, params, {255, 10}, rows) && !rows.empty())
    {
        std::string dbPassword = rows[0][0];
        int isActive = std::stoi(rows[0][1]);

        if (isActive == 0) {
            std::cout << "[AdminAuthService] 로그인 실패 (비활성화된 유저): " << userId << "\n";
            outMsg = "비활성화된 관리자 계정입니다.";
            DBConnectionPool::getInstance().releaseConnection(conn);
            return false;
        }

        if (bcryptVerify(inputPw, dbPassword))
        {
            std::cout << "[AdminAuthService] 관리자 로그인 성공: " << userId << "\n";
            success = true;
        }
        else
        {
            std::cout << "[AdminAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
            outMsg = "아이디/비밀번호가 일치하지 않습니다.";
        }
    }
    else
    {
        std::cout << "[AdminAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
        outMsg = "존재하지 않는 관리자 아이디입니다.";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

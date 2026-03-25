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
bool AdminAuthService::processLogin(int fd, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId = payload["userId"];
    std::string inputPw = payload["password"]; // 클라이언트 SHA256 해싱값

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string query = "SELECT password FROM admin_log WHERE email = ? AND is_active = 1";
    std::vector<std::string> params = {userId};
    std::string dbPassword; // DB에 저장된 bcrypt 해시

    bool success = false;

    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword))
    {
        // ✅ 수정: == 단순 비교 → bcryptVerify로 검증
        if (bcryptVerify(inputPw, dbPassword))
        {
            std::cout << "[AdminAuthService] 관리자 로그인 성공: " << userId << "\n";
            success = true;
        }
        else
        {
            std::cout << "[AdminAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
        }
    }
    else
    {
        std::cout << "[AdminAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

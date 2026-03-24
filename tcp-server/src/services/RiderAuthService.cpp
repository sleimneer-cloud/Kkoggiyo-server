#include "services/RiderAuthService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <crypt.h>        // crypt() — bcrypt 지원 (링크: -lcrypt)
#include <openssl/rand.h> // RAND_bytes() — 암호학적 난수 (링크: -lssl -lcrypto)

// ---------------------------------------------------------
// bcrypt 유틸 (CustomerAuthService와 동일한 구현)
// ---------------------------------------------------------

static std::string generateBcryptSalt()
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

static std::string bcryptHash(const std::string &password)
{
    std::string salt = generateBcryptSalt();
    const char *hashed = crypt(password.c_str(), salt.c_str());
    return hashed ? std::string(hashed) : "";
}

static bool bcryptVerify(const std::string &password, const std::string &hash)
{
    if (hash.empty()) return false;
    const char *result = crypt(password.c_str(), hash.c_str());
    return result && (hash == std::string(result));
}

// ---------------------------------------------------------
// 1. 회원가입 (INSERT)
// ---------------------------------------------------------
bool RiderAuthService::processRegister(const nlohmann::json &payload)
{
    // 라이더 가입에 필요한 필드 검증
    // phone: 배달 연락용 필수값
    if (!payload.contains("userId")   ||
        !payload.contains("password") ||
        !payload.contains("userName") ||
        !payload.contains("phone"))
    {
        std::cerr << "[RiderAuthService] 회원가입 필수 필드 누락\n";
        return false;
    }

    std::string userId   = payload["userId"];
    std::string password = payload["password"]; // 클라이언트 SHA256 해싱값
    std::string userName = payload["userName"];
    std::string phone    = payload["phone"];

    // 서버에서 bcrypt로 한 번 더 해싱
    std::string hashedPassword = bcryptHash(password);
    if (hashedPassword.empty())
    {
        std::cerr << "[RiderAuthService] bcrypt 해싱 실패: " << userId << "\n";
        return false;
    }

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    // role = 3 (RIDER) 으로 INSERT
    // member 테이블 구조: login_id, password, name, role, phone
    std::string query = "INSERT INTO member (login_id, password, name, role, phone) VALUES (?, ?, ?, 3, ?)";
    std::vector<std::string> params = {userId, hashedPassword, userName, phone};

    if (!DBHelper::executeUpdate(conn, query, params))
    {
        std::cerr << "[RiderAuthService] 회원가입 실패: " << userId << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    std::cout << "[RiderAuthService] 회원가입 완료: " << userId << "\n";
    DBConnectionPool::getInstance().releaseConnection(conn);
    return true;
}

// ---------------------------------------------------------
// 2. 로그인 (SELECT + bcrypt 검증)
// ---------------------------------------------------------
bool RiderAuthService::processLogin(int fd, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId   = payload["userId"];
    std::string inputPw  = payload["password"]; // 클라이언트 SHA256 해싱값

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    // role = 3 (RIDER) 인 계정만 조회 — 다른 role로 라이더 로그인 불가
    std::string query = "SELECT password FROM member WHERE login_id = ? AND role = 3";
    std::vector<std::string> params = {userId};
    std::string dbPassword;

    bool success = false;

    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword))
    {
        if (bcryptVerify(inputPw, dbPassword))
        {
            std::cout << "[RiderAuthService] 라이더 로그인 성공: " << userId << "\n";
            success = true;
        }
        else
        {
            std::cout << "[RiderAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
        }
    }
    else
    {
        std::cout << "[RiderAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

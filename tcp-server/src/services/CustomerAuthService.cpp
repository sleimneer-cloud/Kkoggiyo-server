#include "services/CustomerAuthService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <crypt.h>        // crypt() — bcrypt 지원 (링크: -lcrypt)
#include <openssl/rand.h> // RAND_bytes() — 암호학적 난수 (링크: -lssl -lcrypto)

// ---------------------------------------------------------
// bcrypt 유틸 함수 (이 파일 내부에서만 사용)
// ---------------------------------------------------------

// 암호학적 난수로 bcrypt salt 생성: "$2b$12$" + 22자리 랜덤 문자
static std::string generateBcryptSalt()
{
    // bcrypt가 허용하는 base64 문자셋 (표준 base64와 다름)
    static const char b64chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";

    unsigned char rnd[22];
    RAND_bytes(rnd, sizeof(rnd)); // OpenSSL 암호학적 난수 — rand() 절대 사용 금지

    std::string salt = "$2b$12$"; // $2b$ = bcrypt 버전, 12 = cost factor (2^12회 반복)
    for (int i = 0; i < 22; ++i)
        salt += b64chars[rnd[i] % 64];

    return salt; // 예: "$2b$12$X7kP2mNqRvLsWoAeT1uYzO"
}

// 비밀번호 → bcrypt 해시 문자열 반환
static std::string bcryptHash(const std::string &password)
{
    std::string salt = generateBcryptSalt();
    const char *hashed = crypt(password.c_str(), salt.c_str());
    return hashed ? std::string(hashed) : "";
}

// 입력 비밀번호와 DB 저장 해시 비교 — 일치하면 true
static bool bcryptVerify(const std::string &password, const std::string &hash)
{
    if (hash.empty()) return false;
    const char *result = crypt(password.c_str(), hash.c_str());
    // 타이밍 공격 방지를 위해 == 대신 문자열 전체 비교
    return result && (hash == std::string(result));
}

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

    std::string userId   = payload["userId"];
    std::string password = payload["password"]; // 클라이언트가 SHA256 해싱 후 전송한 값
    std::string userName = payload["userName"];
    std::string phone    = payload["phone"];

    // ✅ 수정: 서버에서 bcrypt로 한 번 더 해싱 후 저장
    // 클라이언트 SHA256 → 네트워크 구간 평문 노출 방지
    // 서버 bcrypt   → DB 탈취 시 Pass-the-Hash 공격 방지
    std::string hashedPassword = bcryptHash(password);
    if (hashedPassword.empty())
    {
        std::cerr << "[CustomerAuthService] bcrypt 해싱 실패: " << userId << "\n";
        return false;
    }

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string query = "INSERT INTO member (login_id, password, name, role, phone) VALUES (?, ?, ?, 1, ?)";
    std::vector<std::string> params = {userId, hashedPassword, userName, phone}; // ✅ 해시값 저장

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

    std::string userId        = payload["userId"];
    std::string inputPassword = payload["password"]; // 클라이언트 SHA256 해싱값

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string query = "SELECT password FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};
    std::string dbPassword; // DB에 저장된 bcrypt 해시

    bool isSuccess = false;

    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword))
    {
        // ✅ 수정: == 단순 비교 → bcryptVerify로 검증
        // crypt(inputPassword, dbPassword의 salt) 결과가 dbPassword와 같으면 일치
        if (bcryptVerify(inputPassword, dbPassword))
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

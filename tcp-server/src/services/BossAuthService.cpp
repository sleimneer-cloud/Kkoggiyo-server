#include "services/BossAuthService.hpp"
#include <iostream>
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <crypt.h>        // crypt() — bcrypt 검증 (링크: -lcrypt)
#include <openssl/rand.h> // bcrypt 솔트 생성 (링크: -lssl -lcrypto)

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

// ---------------------------------------------------------
// 로그인 로직
// ---------------------------------------------------------
bool BossAuthService::processLogin(int fd, const nlohmann::json &payload, std::string &outMsg)
{
    if (!payload.contains("userId") || !payload.contains("password")) {
        outMsg = "로그인 정보가 누락되었습니다.";
        return false;
    }

    std::string userId  = payload["userId"];
    std::string inputPw = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    std::string query = "SELECT password, is_banned FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> params = {userId};

    bool success = false;
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(conn, query, params, {255, 10}, rows) && !rows.empty())
    {
        std::string dbPassword = rows[0][0];
        int isBanned = std::stoi(rows[0][1]);

        if (isBanned == 1) {
            std::cout << "[BossAuthService] 로그인 실패 (차단된 유저): " << userId << "\n";
            outMsg = "관리자에 의해 정지(차단)된 계정입니다.";
            DBConnectionPool::getInstance().releaseConnection(conn);
            return false;
        }

        if (bcryptVerify(inputPw, dbPassword))
        {
            std::cout << "[BossAuthService] 사장님 로그인 성공: " << userId << "\n";
            success = true;
        }
        else
        {
            std::cout << "[BossAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
            outMsg = "아이디/비밀번호가 일치하지 않습니다.";
        }
    }
    else
    {
        std::cout << "[BossAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
        outMsg = "존재하지 않는 아이디입니다.";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return success;
}

// ---------------------------------------------------------
// 회원가입 로직
// ---------------------------------------------------------
bool BossAuthService::processRegister(const nlohmann::json &payload)
{
    // ▼ 수정: storeName, storeAddress, licnese 필드 검증 추가
    if (!payload.contains("userId")       || !payload.contains("password") ||
        !payload.contains("userName")     || !payload.contains("phone")    ||
        !payload.contains("storeName")    || !payload.contains("storeAddress") ||
        !payload.contains("license"))
    {
        return false;
    }

    std::string userId       = payload["userId"];
    std::string password     = payload["password"]; // 클라이언트가 보낸 SHA256 해시값
    std::string userName     = payload["userName"];
    std::string phone        = payload["phone"];
    // ▼ 수정: 가게 정보 추출 추가
    std::string storeName    = payload["storeName"];
    std::string storeAddress = payload["storeAddress"];
    // license 필드 (기존 licnese 오타 수정됨)
    std::string license      = payload["license"];

    // 서버에서 bcrypt로 한 번 더 강력하게 해싱
    std::string hashedPassword = bcryptHash(password);
    if (hashedPassword.empty())
    {
        std::cerr << "[BossAuthService] bcrypt 해싱 실패: " << userId << "\n";
        return false;
    }

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    // ── 1단계: member 테이블에 사장님 계정 저장 (role = 2) ──────────────────
    std::string memberQuery = "INSERT INTO member (login_id, password, name, role, phone) VALUES (?, ?, ?, 2, ?)";
    std::vector<std::string> memberParams = {userId, hashedPassword, userName, phone};

    if (!DBHelper::executeUpdate(conn, memberQuery, memberParams))
    {
        std::cerr << "[BossAuthService] 사장님 회원가입 실패 (member): " << userId << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // ── 2단계: store 테이블에 가게 정보 저장 ────────────────────────────────
    // LAST_INSERT_ID()로 방금 INSERT된 member의 PK를 FK로 사용합니다.
    // store 테이블의 컬럼명(name, address, license)은 실제 스키마에 맞게 조정하세요.
    std::string storeQuery = "INSERT INTO store (owner_id, store_name, store_address, business_reg_num) VALUES (LAST_INSERT_ID(), ?, ?, ?)";
    std::vector<std::string> storeParams = {storeName, storeAddress, license};

    if (!DBHelper::executeUpdate(conn, storeQuery, storeParams))
    {
        std::cerr << "[BossAuthService] 가게 정보 저장 실패 (store): " << userId << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    std::cout << "[BossAuthService] 사장님 회원가입 완료 (member + store): " << userId << "\n";
    DBConnectionPool::getInstance().releaseConnection(conn);
    return true;
}

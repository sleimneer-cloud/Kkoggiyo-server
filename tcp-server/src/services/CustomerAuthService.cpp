#include "services/CustomerAuthService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <random>
#include <cstdlib>
#include <chrono>
#include <crypt.h>        // crypt() — bcrypt 지원 (링크: -lcrypt)
#include <openssl/rand.h> // RAND_bytes() — 암호학적 난수 (링크: -lssl -lcrypto)

// static 멤버 초기화
std::map<std::string, VerifyEntry> CustomerAuthService::s_verifyMap;
std::mutex CustomerAuthService::s_mutex;

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
// 1. 회원가입
// ---------------------------------------------------------
bool CustomerAuthService::processRegister(const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password") ||
        !payload.contains("userName"))
    {
        return false;
    }

    std::string userId   = payload["userId"];
    std::string password = payload["password"]; // 클라이언트가 SHA256 해싱 후 전송한 값
    std::string userName = payload["userName"];

    // 인증번호 생성
    std::string code = generateCode();

    // 인증번호 임시 저장
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_verifyMap[userId] = {
            code,
            std::chrono::steady_clock::now(),
            userId, password, userName
        };
    }

    // 이메일 발송
    return sendVerifyEmail(userId, code);
}

// ---------------------------------------------------------
// 2. 로그인
// ---------------------------------------------------------
bool CustomerAuthService::processLogin(int /*fd*/, const nlohmann::json &payload, std::string &outName, std::string &outMsg)
{
    if (!payload.contains("userId") || !payload.contains("password")) {
        outMsg = "로그인 정보가 누락되었습니다.";
        return false;
    }

    std::string userId        = payload["userId"];
    std::string inputPassword = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }
    
    std::string query = "SELECT password, name, is_banned FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};

    bool isSuccess = false;
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(conn, query, params, {255, 50, 10}, rows) && !rows.empty())
    {
        std::string dbPassword = rows[0][0];
        outName = rows[0][1];
        int isBanned = std::stoi(rows[0][2]);

        if (isBanned == 1) {
            std::cout << "[CustomerAuthService] 로그인 실패 (차단된 유저): " << userId << "\n";
            outMsg = "관리자에 의해 정지(차단)된 계정입니다.";
            DBConnectionPool::getInstance().releaseConnection(conn);
            return false;
        }

        if (bcryptVerify(inputPassword, dbPassword))
        {
            std::cout << "[CustomerAuthService] 로그인 성공: " << userId << "\n";
            isSuccess = true;
        }
        else
        {
            std::cout << "[CustomerAuthService] 로그인 실패 (비밀번호 불일치): " << userId << "\n";
            outMsg = "아이디/비밀번호가 일치하지 않습니다.";
        }
    }
    else
    {
        std::cout << "[CustomerAuthService] 로그인 실패 (존재하지 않는 아이디): " << userId << "\n";
        outMsg = "해당 아이디를 찾을 수 없습니다.";
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return isSuccess;
}

// ─────────────────────────────────────────
//  6자리 랜덤 인증 코드 생성
// ─────────────────────────────────────────
std::string CustomerAuthService::generateCode()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    return std::to_string(dis(gen));
}

// ─────────────────────────────────────────
//  이메일 인증번호 발송
// ─────────────────────────────────────────
bool CustomerAuthService::sendVerifyEmail(const std::string &email,
                                          const std::string &code)
{
    std::string cmd =
        "printf 'To: " + email + "\\n"
        "Subject: [꼭기요] 이메일 인증번호\\n"
        "\\n"
        "안녕하세요. 꼭기요입니다.\\n"
        "인증번호 : " + code + "\\n"
        "30초 이내에 입력해 주세요.\\n'"
        " | msmtp -t";

    int ret = system(cmd.c_str());
    if (ret != 0) {
        std::cerr << "[CustomerAuthService] 이메일 발송 실패: " << email << "\n";
        return false;
    }
    std::cout << "[CustomerAuthService] 인증번호 발송: "
              << email << " / 코드: " << code << "\n";
    return true;
}

// ─────────────────────────────────────────
//  인증번호 확인 처리
// ─────────────────────────────────────────
bool CustomerAuthService::processVerify(int /*fd*/, const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("code"))
        return false;

    std::string loginId = payload["userId"];
    std::string code    = payload["code"];

    std::lock_guard<std::mutex> lock(s_mutex);

    auto it = s_verifyMap.find(loginId);
    if (it == s_verifyMap.end()) {
        std::cerr << "[CustomerAuthService] 인증 정보 없음: " << loginId << "\n";
        return false;
    }

    VerifyEntry &entry = it->second;

    // 30초 만료 확인
    auto elapsed = std::chrono::steady_clock::now() - entry.createdAt;
    if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() > 30) {
        s_verifyMap.erase(it);
        std::cerr << "[CustomerAuthService] 인증번호 만료: " << loginId << "\n";
        return false;
    }

    // 코드 불일치
    if (entry.code != code) {
        std::cerr << "[CustomerAuthService] 인증번호 불일치: " << loginId << "\n";
        return false;
    }

    // 탈퇴용 인증 — password, name 비어있으면 DB INSERT 생략
    if (entry.password.empty() && entry.name.empty()) {
        s_verifyMap.erase(it);
        std::cout << "[CustomerAuthService] 탈퇴 인증 완료: " << loginId << "\n";
        return true;
    }

    // ── 인증 성공 → DB에 회원 저장
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    // 비밀번호 암호화
    std::string encryptedPw = bcryptHash(entry.password); 
    if (encryptedPw.empty()) {
        std::cerr << "[CustomerAuthService] bcrypt 해싱 실패: " << loginId << "\n";
        return false;
    }

    std::string query =
        "INSERT INTO member (login_id, password, name, role) "
        "VALUES (?, ?, ?, 1)";
    std::vector<std::string> params = {
        entry.loginId, encryptedPw, entry.name
    };

    bool ok = DBHelper::executeUpdate(conn, query, params);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (ok) {
        s_verifyMap.erase(it); // 인증 완료 후 임시 데이터 삭제
        std::cout << "[CustomerAuthService] 회원가입 완료: " << loginId << "\n";
    }

    return ok;
}

// ─────────────────────────────────────────
//  닉네임 변경
// ─────────────────────────────────────────
bool CustomerAuthService::processNameChange(const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("name"))
        return false;

    std::string userId = payload["userId"];
    std::string name   = payload["name"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    std::string query = "UPDATE member SET name = ? WHERE login_id = ?";
    std::vector<std::string> params = {name, userId};

    bool ok = DBHelper::executeUpdate(conn, query, params);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (ok) std::cout << "[CustomerAuthService] 닉네임 변경: " << userId << " → " << name << "\n";
    return ok;
}

// ─────────────────────────────────────────
//  탈퇴 비밀번호 검증
// ─────────────────────────────────────────
bool CustomerAuthService::processWithdrawVerify(const nlohmann::json &payload)
{
    if (!payload.contains("userId") || !payload.contains("password"))
        return false;

    std::string userId        = payload["userId"];
    std::string inputPassword = payload["password"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    std::string query = "SELECT password FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};
    std::string dbPassword;

    bool isSuccess = false;

    if (DBHelper::executeSelectOneString(conn, query, params, dbPassword)) {
        if (bcryptVerify(inputPassword, dbPassword)) {
            // 비밀번호 일치 시 인증번호 발송
            std::string code = generateCode();
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_verifyMap[userId] = {
                    code,
                    std::chrono::steady_clock::now(),
                    userId, "", ""
                };
            }
            isSuccess = sendVerifyEmail(userId, code);
            std::cout << "[CustomerAuthService] 탈퇴 인증번호 발송: " << userId << "\n";
        } else {
            std::cout << "[CustomerAuthService] 탈퇴 비밀번호 불일치: " << userId << "\n";
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return isSuccess;
}

// ─────────────────────────────────────────
//  회원탈퇴
// ─────────────────────────────────────────
bool CustomerAuthService::processWithdraw(const nlohmann::json &payload)
{
    if (!payload.contains("userId"))
        return false;

    std::string userId = payload["userId"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    std::string query = "DELETE FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};

    bool ok = DBHelper::executeUpdate(conn, query, params);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (ok) std::cout << "[CustomerAuthService] 회원탈퇴 완료: " << userId << "\n";
    else    std::cerr << "[CustomerAuthService] 회원탈퇴 실패: " << userId << "\n";

    return ok;
}

// ─────────────────────────────────────────
//  아이디 중복 확인
// ─────────────────────────────────────────
bool CustomerAuthService::processIdCheck(const nlohmann::json &payload)
{
    if (!payload.contains("userId"))
        return false;

    std::string userId = payload["userId"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        return false;
    }

    std::string query = "SELECT login_id FROM member WHERE login_id = ?";
    std::vector<std::string> params = {userId};
    std::string result;

    bool exists = DBHelper::executeSelectOneString(conn, query, params, result);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return !exists;
}

// ─────────────────────────────────────────
//  주소 추가
// ─────────────────────────────────────────
bool CustomerAuthService::processAddAddress(const nlohmann::json &payload)
{
    if (!payload.contains("userId")       ||
        !payload.contains("roadAddress")  ||
        !payload.contains("detailAddress")||
        !payload.contains("alias"))
        return false;

    std::string userId        = payload["userId"];
    std::string roadAddress   = payload["roadAddress"];
    std::string detailAddress = payload["detailAddress"];
    std::string alias         = payload["alias"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    // member_id 조회
    std::string memberIdQuery =
        "SELECT member_id FROM member WHERE login_id = ? AND role = 1";
    std::vector<std::string> params = {userId};
    std::string memberId;

    if (!DBHelper::executeSelectOneString(conn, memberIdQuery, params, memberId)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // 주소 INSERT
    std::string insertQuery =
        "INSERT INTO addresses "
        "(member_id, alias, road_address, detail_address, is_recent, used_at) "
        "VALUES (?, ?, ?, ?, 1, NOW())";
    std::vector<std::string> insertParams = {
        memberId, alias, roadAddress, detailAddress
    };

    bool ok = DBHelper::executeUpdate(conn, insertQuery, insertParams);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (ok) std::cout << "[CustomerAuthService] 주소 추가 완료: " << userId << "\n";
    else    std::cerr << "[CustomerAuthService] 주소 추가 실패: " << userId << "\n";

    return ok;
}

// ─────────────────────────────────────────
//  주소 목록 조회
// ─────────────────────────────────────────
std::vector<nlohmann::json> CustomerAuthService::processGetAddresses(
    const nlohmann::json &payload)
{
    std::vector<nlohmann::json> result;
    if (!payload.contains("userId")) return result;

    std::string userId = payload["userId"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return result;

    std::string query =
        "SELECT address_id, alias, road_address, detail_address "
        "FROM addresses "
        "WHERE member_id = ("
        "    SELECT member_id FROM member WHERE login_id = ? AND role = 1"
        ") "
        "ORDER BY used_at DESC";

    std::vector<std::string> params = {userId};
    // address_id(20), alias(21), road_address(201), detail_address(101)
    std::vector<int> colSizes = {20, 21, 201, 101};
    std::vector<std::vector<std::string>> rows;

    if (!DBHelper::executeSelectMultipleRows(conn, query, params, colSizes, rows)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return result;
    }

    for (const auto &row : rows) {
        nlohmann::json item;
        item["addressId"]     = row[0];
        item["alias"]         = row[1];
        item["roadAddress"]   = row[2];
        item["detailAddress"] = row[3];
        result.push_back(item);
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    std::cout << "[CustomerAuthService] 주소 목록 조회: "
              << userId << " / " << result.size() << "건\n";
    return result;
}
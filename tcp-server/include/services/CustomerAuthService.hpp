#pragma once // 중복 포함 방지

#include "../json.hpp" // JSON 라이브러리 포함 (경로는 프로젝트 트리에 맞게 조절)
#include <string>
#include <map>
#include <mutex>
#include <chrono>

// 사용자 정보 임시 저장 구조체
struct VerifyEntry {
    std::string code;
    std::chrono::steady_clock::time_point createdAt;
    std::string loginId;
    std::string password;
    std::string name;
};

class CustomerAuthService
{
public:
    CustomerAuthService() = default;
    ~CustomerAuthService() = default;

    // 고객 로그인
    bool processLogin(int fd, const nlohmann::json &payload, std::string &outName);
    // 회원가입 시 인증번호 발송
    bool processRegister(const nlohmann::json &payload);    
    // 인증번호 확인 및 DB 저장
    bool processVerify(int fd, const nlohmann::json &payload);
    // 닉네임 변경
    bool processNameChange(const nlohmann::json &payload);
    // 탈퇴 비밀번호 검증
    bool processWithdrawVerify(const nlohmann::json &payload);
    // 회원탈퇴 DB 삭제
    bool processWithdraw(const nlohmann::json &payload);
    // 아이디 중복 확인
    bool processIdCheck(const nlohmann::json &payload);
    // 주소 추가
    bool processAddAddress(const nlohmann::json &payload);
    // 주소 목록 조회
    std::vector<nlohmann::json> processGetAddresses(const nlohmann::json &payload);

private:
    std::string generateCode();  // 6자리 랜덤 코드 생성
    bool sendVerifyEmail(const std::string &email,
                         const std::string &code); // 이메일 발송
    // 인증 코드 임시 저장소
    static std::map<std::string, VerifyEntry> s_verifyMap;
    static std::mutex s_mutex;
};
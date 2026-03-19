#pragma once // 중복 포함 방지

#include "../json.hpp" // JSON 라이브러리 포함 (경로는 프로젝트 트리에 맞게 조절)
#include <string>

class CustomerAuthService
{
public:
    CustomerAuthService() = default;
    ~CustomerAuthService() = default;

    // 고객 로그인 처리를 담당할 함수
    // 성공하면 true, 실패하면 false를 반환합니다.
    bool processLogin(int fd, const nlohmann::json &payload);

    // 나중에 추가할 회원가입 함수 (지금은 선언만)
    bool processRegister(const nlohmann::json &payload);
};
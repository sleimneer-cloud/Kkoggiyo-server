#pragma once
#include "../json.hpp"

// 라이더 인증 서비스
// role = 3 (RIDER) 전용 로그인 / 회원가입 처리
// CustomerAuthService, BossAuthService와 동일한 인터페이스 유지
class RiderAuthService
{
public:
    bool processLogin(int fd, const nlohmann::json &payload);
    bool processRegister(const nlohmann::json &payload);
};

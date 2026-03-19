#pragma once

#include "json.hpp"

// 로그인/인증 처리 (현재는 DB 없이 세션 등록만)
class AuthHandler
{
public:
    void handleLogin(int client_fd, const nlohmann::json &j) const;

    void handleRegister(int client_fd, const nlohmann::json &j) const;
};

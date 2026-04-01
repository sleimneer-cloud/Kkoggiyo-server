#pragma once

#include "json.hpp"

// 계정 정보 관리
class AuthHandler
{
public:
    void handleLogin(int client_fd, const nlohmann::json &j) const;
    void handleVerify(int client_fd, const nlohmann::json &j) const;
    void handleRegister(int client_fd, const nlohmann::json &j) const;
    void handleNameChange(int client_fd, const nlohmann::json &j) const;
    void handleWithdrawVerify(int client_fd, const nlohmann::json &j) const;
    void handleWithdraw(int client_fd, const nlohmann::json &j) const;
    void handleIdCheck(int client_fd, const nlohmann::json &j) const;
    void handleAddAddress(int client_fd, const nlohmann::json &j) const;
    void handleGetAddresses(int client_fd, const nlohmann::json &j) const;
};

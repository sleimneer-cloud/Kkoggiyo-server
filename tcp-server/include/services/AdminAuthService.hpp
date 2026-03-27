#pragma once
#include "../json.hpp"

class AdminAuthService
{
public:
    bool processLogin(int fd, const nlohmann::json &payload, std::string &outMsg);
    bool processRegister(const nlohmann::json &payload);
};
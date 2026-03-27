#pragma once
#include "../json.hpp"

class BossAuthService
{
public:
    bool processLogin(int fd, const nlohmann::json &payload, std::string &outMsg);
    bool processRegister(const nlohmann::json &payload);
};
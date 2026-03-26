#pragma once

#include "json.hpp"

using json = nlohmann::json;

class AdminService
{
public:
    AdminService() = default;
    ~AdminService() = default;

    // DB에서 특정 유저 정보를 조회하여 JSON으로 반환하는 메서드
    json getUserById(int targetId);
};

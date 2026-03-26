#pragma once

#include "json.hpp"
#include <string> // ← [수정] string 타입 사용을 위해 추가

using json = nlohmann::json;

class AdminService
{
public:
    AdminService() = default;
    ~AdminService() = default;

    // 기존: member_id 로 조회
    json getUserById(int targetId);

    // [수정] 신규: login_id 문자열로 조회
    json getUserByLoginId(const std::string& loginId);

    // [추가] 유저의 주문 기록(주문 및 상세 품목) 조회
    json getOrderHistoryByTargetId(int targetId);
    json getOrderHistoryByLoginId(const std::string& loginId);
};

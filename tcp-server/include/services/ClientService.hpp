#pragma once

#include <vector>
#include "json.hpp"

class ClientService {
public:
    ClientService() = default;
    ~ClientService() = default;

    // 가게 목록 조회
    std::vector<nlohmann::json> processGetStores(const nlohmann::json& payload);

    // 메뉴 목록 조회
    std::vector<nlohmann::json> processGetMenus(const nlohmann::json& payload);

    // 주문 전송
    nlohmann::json processPlaceOrder(const nlohmann::json& payload);

    // 주문 내역 조회
    std::vector<nlohmann::json> processGetOrderList(const nlohmann::json& payload);
};

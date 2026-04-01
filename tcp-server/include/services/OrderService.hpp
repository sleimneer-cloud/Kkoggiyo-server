#pragma once
#include "json.hpp"
#include <string>
#include <vector>

using json = nlohmann::json;

class OrderService
{
public:
    // 주문 생성 → 생성된 order_id 반환 (실패 시 -1)
    int createOrder(int customerId, int storeId,
                    const std::string& requestMsg,
                    const json& items);

    // 주문 상태 변경
    bool updateStatus(int orderId, const std::string& status);

    // 주문 단건 조회
    json getOrder(int orderId);

    // 고객 주문 이력 조회
    json getOrdersByCustomer(int customerId);

    // 가게 주문 목록 조회 (진행 중인 주문)
    json getOrdersByStore(int storeId);

    // 사장님용 상세 주문 내역 조회 (아이템 포함)
    json getOrdersWithItemsByStore(int storeId);

    // 로그인 아이디로 가게 ID 리스트 조회 (여러 개 대응)
    std::vector<int> getStoreIdsByOwnerLoginId(const std::string& loginId);

    // 로그인 아이디로 첫 번째 가게 ID 조회 (기존 호환용)
    int getStoreIdByOwnerLoginId(const std::string& loginId);

    // 라이더용 배달 가능 주문 조회 (COOKING, READY)
    json getAvailableOrdersForRider();

    // 라이더 배정 (배차 완료 처리용)
    bool assignRiderToOrder(int orderId, int riderId);

    // 라이더 배정
    bool assignRider(int orderId, int riderId);

    // 라이더 배정 해제
    bool unassignRider(int orderId);
};

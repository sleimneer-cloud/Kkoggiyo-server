#pragma once
#include "json.hpp"
#include "SessionManager.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"

using json = nlohmann::json;

class NotifyService
{
public:
    // 가게에 신규 주문 알림
    void notifyStore(int storeId, int orderId, const json& orderInfo);

    // 고객에게 주문 상태 변경 알림
    void notifyCustomer(int customerId, int orderId, const std::string& status);

    // 라이더에게 배달 요청 알림
    void notifyRider(int riderId, int orderId, const json& orderInfo);

    // 라이더 전체에게 배달 요청(콜) 알림
    void broadcastToRiders(int orderId, const json& orderInfo);

    // [추가] 유저에게 차단 알림 및 강제 종료
    void notifyBan(int memberId, const std::string& reason);

    // [추가] 사장님과 고객에게 배차 완료 알림
    void notifyDispatchComplete(int orderId);
};

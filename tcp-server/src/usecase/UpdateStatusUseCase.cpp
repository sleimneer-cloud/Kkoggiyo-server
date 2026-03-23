#include "usecase/UpdateStatusUseCase.hpp"
#include <iostream>

bool UpdateStatusUseCase::execute(int orderId, const std::string& status)
{
    // 1. 주문 조회
    json order = orderSvc_.getOrder(orderId);
    if (order.empty()) return false;

    // 2. 상태 업데이트
    if (!orderSvc_.updateStatus(orderId, status))
        return false;

    int customerId = order.value("customer_id", 0);
    int storeId    = order.value("store_id",    0);
    int riderId    = order.value("rider_id",    0);

    // 3. 상태별 알림 대상 결정
    // READY      → 라이더에게 픽업 요청
    // DELIVERING → 고객에게 배달 시작 알림
    // COMPLETED  → 고객 + 가게 완료 알림
    if (status == "READY" && riderId > 0)
    {
        notifySvc_.notifyRider(riderId, orderId,
            {{"store_id", storeId}, {"message", "픽업 요청입니다."}});
    }
    else if (status == "DELIVERING")
    {
        notifySvc_.notifyCustomer(customerId, orderId, status);
    }
    else if (status == "COMPLETED")
    {
        notifySvc_.notifyCustomer(customerId, orderId, status);
        notifySvc_.notifyStore(storeId, orderId,
            {{"message", "배달이 완료되었습니다."}});
    }
    else
    {
        notifySvc_.notifyCustomer(customerId, orderId, status);
    }

    std::cout << "[UpdateStatusUseCase] 상태 변경 완료. order_id: "
              << orderId << " / status: " << status << "\n";
    return true;
}

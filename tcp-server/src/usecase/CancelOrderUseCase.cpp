#include "usecase/CancelOrderUseCase.hpp"
#include <iostream>

bool CancelOrderUseCase::execute(int orderId, int requesterId)
{
    // 1. 주문 정보 조회
    json order = orderSvc_.getOrder(orderId);
    if (order.empty())
    {
        std::cerr << "[CancelOrderUseCase] 주문 없음. order_id: " << orderId << "\n";
        return false;
    }

    // 2. 이미 완료/취소된 주문은 취소 불가
    std::string status = order.value("status", "");
    if (status == "COMPLETED" || status == "CANCELLED")
    {
        std::cerr << "[CancelOrderUseCase] 취소 불가 상태: " << status << "\n";
        return false;
    }

    // 3. 상태를 CANCELLED로 변경
    if (!orderSvc_.updateStatus(orderId, "CANCELLED"))
        return false;

    // 4. 고객에게 취소 알림
    int customerId = order.value("customer_id", 0);
    notifySvc_.notifyCustomer(customerId, orderId, "CANCELLED");

    // 5. 라이더가 배정된 경우 배정 해제 후 알림
    int riderId = order.value("rider_id", 0);
    if (riderId > 0)
    {
        orderSvc_.unassignRider(orderId);
        notifySvc_.notifyRider(riderId, orderId,
            {{"message", "주문이 취소되었습니다."}});
    }

    std::cout << "[CancelOrderUseCase] 주문 취소 완료. order_id: " << orderId << "\n";
    return true;
}

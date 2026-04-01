#pragma once
#include "services/OrderService.hpp"
#include "services/NotifyService.hpp"

class AcceptOrderUseCase
{
public:
    AcceptOrderUseCase(OrderService& orderSvc, NotifyService& notifySvc)
        : orderSvc_(orderSvc), notifySvc_(notifySvc) {}

    // accepted: true=수락, false=거절
    // estimatedTime: 예상 조리 시간 (분)
    bool execute(int orderId, int estimatedTime, bool accepted);

private:
    OrderService&  orderSvc_;
    NotifyService& notifySvc_;
};

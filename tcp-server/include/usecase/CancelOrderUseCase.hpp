#pragma once
#include "services/OrderService.hpp"
#include "services/NotifyService.hpp"

class CancelOrderUseCase
{
public:
    CancelOrderUseCase(OrderService& orderSvc, NotifyService& notifySvc)
        : orderSvc_(orderSvc), notifySvc_(notifySvc) {}

    // requesterId: 취소 요청자의 member_id
    bool execute(int orderId, int requesterId);

private:
    OrderService&  orderSvc_;
    NotifyService& notifySvc_;
};

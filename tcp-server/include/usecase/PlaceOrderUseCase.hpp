#pragma once
#include "services/OrderService.hpp"
#include "services/NotifyService.hpp"

class PlaceOrderUseCase
{
public:
    PlaceOrderUseCase(OrderService& orderSvc, NotifyService& notifySvc)
        : orderSvc_(orderSvc), notifySvc_(notifySvc) {}

    // 성공 시 order_id 반환, 실패 시 -1
    int execute(int customerId, int storeId,
                const std::string& requestMsg,
                const json& items);

private:
    OrderService&  orderSvc_;
    NotifyService& notifySvc_;
};

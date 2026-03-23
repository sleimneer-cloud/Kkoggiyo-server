#pragma once
#include "services/OrderService.hpp"
#include "services/NotifyService.hpp"

class UpdateStatusUseCase
{
public:
    UpdateStatusUseCase(OrderService& orderSvc, NotifyService& notifySvc)
        : orderSvc_(orderSvc), notifySvc_(notifySvc) {}

    // status: "READY" | "DELIVERING" | "COMPLETED"
    bool execute(int orderId, const std::string& status);

private:
    OrderService&  orderSvc_;
    NotifyService& notifySvc_;
};

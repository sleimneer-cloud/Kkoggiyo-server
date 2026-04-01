#include "usecase/AcceptOrderUseCase.hpp"
#include "DBConnectionPool.h"
#include <mysql/mysql.h>
#include <iostream>

bool AcceptOrderUseCase::execute(int orderId, int estimatedTime, bool accepted)
{
    // 1. 주문 조회
    json order = orderSvc_.getOrder(orderId);
    if (order.empty()) return false;

    int customerId = order.value("customer_id", 0);

    if (!accepted)
    {
        // 거절 → CANCELLED 처리
        orderSvc_.updateStatus(orderId, "CANCELLED");
        notifySvc_.notifyCustomer(customerId, orderId, "CANCELLED");
        std::cout << "[AcceptOrderUseCase] 주문 거절. order_id: " << orderId << "\n";
        return true;
    }

    // 2. 수락 → COOKING 상태로 변경
    if (!orderSvc_.updateStatus(orderId, "COOKING"))
        return false;

    // 3. estimated_time 업데이트
    MYSQL* conn = DBConnectionPool::getInstance().getConnection();
    if (conn)
    {
        std::string q = "UPDATE orders SET estimated_time = "
                        + std::to_string(estimatedTime)
                        + " WHERE order_id = " + std::to_string(orderId);
        mysql_query(conn, q.c_str());
        DBConnectionPool::getInstance().releaseConnection(conn);
    }

    // 4. 고객에게 수락 알림
    notifySvc_.notifyCustomer(customerId, orderId, "COOKING");

    std::cout << "[AcceptOrderUseCase] 주문 수락. order_id: " << orderId
              << " / 예상시간: " << estimatedTime << "분\n";
    return true;
}

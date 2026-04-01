#include "usecase/PlaceOrderUseCase.hpp"
#include <iostream>

int PlaceOrderUseCase::execute(int customerId, int storeId,
                                const std::string& requestMsg,
                                const json& items)
{
    // 1. DB에 주문 생성
    int orderId = orderSvc_.createOrder(customerId, storeId, requestMsg, items);
    if (orderId == -1)
    {
        std::cerr << "[PlaceOrderUseCase] 주문 생성 실패\n";
        return -1;
    }

    // 2. 주문 정보 구성
    json orderInfo = {
        {"order_id",    orderId},
        {"customer_id", customerId},
        {"store_id",    storeId},
        {"request_msg", requestMsg},
        {"items",       items}
    };

    // 3. 가게에 신규 주문 알림
    notifySvc_.notifyStore(storeId, orderId, orderInfo);

    std::cout << "[PlaceOrderUseCase] 주문 완료. order_id: " << orderId << "\n";
    return orderId;
}

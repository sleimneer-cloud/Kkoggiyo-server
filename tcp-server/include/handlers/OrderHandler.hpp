#pragma once
#include "json.hpp"
#include "usecase/PlaceOrderUseCase.hpp"
#include "usecase/CancelOrderUseCase.hpp"
#include "usecase/AcceptOrderUseCase.hpp"
#include "usecase/UpdateStatusUseCase.hpp"
#include "services/OrderService.hpp"
#include "services/NotifyService.hpp"

using json = nlohmann::json;

class OrderHandler
{
public:
    OrderHandler();

    void handle(int clientFd, int packetType, const json &j);

private:
    // 서비스 인스턴스 (핸들러가 소유)
    OrderService orderSvc_;
    NotifyService notifySvc_;

    // UseCase 인스턴스
    PlaceOrderUseCase placeOrderUC_;
    CancelOrderUseCase cancelOrderUC_;
    AcceptOrderUseCase acceptOrderUC_;
    UpdateStatusUseCase updateStatusUC_;

    // 패킷 타입별 처리 함수
    void handlePlaceOrder(int clientFd, const json &j); // 300
    void handleCancel(int clientFd, const json &j);     // 304
    void handleAccept(int clientFd, const json &j);     // 305
    void handlePickup(int clientFd, const json &j);     // 306
    void handleReady(int clientFd, const json &j);      // 307
    void handleDone(int clientFd, const json &j);       // 308

    void handleOwnerOrderList(int clientFd, const json &j); // 310
    void handleRiderOrderList(int clientFd, const json &j); // 320
    void handleRiderAssign(int clientFd, const json &j);    // 322
};

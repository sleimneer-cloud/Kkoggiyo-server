#include "handlers/OrderHandler.hpp"
#include "PacketHandler.hpp"
#include "SessionManager.hpp"
#include <iostream>
#include <protocol.hpp>

OrderHandler::OrderHandler() // 이 핸들러는 OrderService와 NotificationService가 필요하므로, 생성자에서 초기화
    : orderSvc_(), notifySvc_(), placeOrderUC_(orderSvc_, notifySvc_), cancelOrderUC_(orderSvc_, notifySvc_), acceptOrderUC_(orderSvc_, notifySvc_), updateStatusUC_(orderSvc_, notifySvc_)
{
}

void OrderHandler::handle(int clientFd, int packetType, const json &j) // 패킷 타입에 따라 적절한 핸들러 함수 호출
{
    switch (static_cast<PacketType>(packetType))
    {
    case PacketType::CS_ORDER_REQ:
        handlePlaceOrder(clientFd, j);
        break;
    case PacketType::CS_ORDER_CANCEL:
        handleCancel(clientFd, j);
        break;
    case PacketType::CS_ORDER_ACCEPT:
        handleAccept(clientFd, j);
        break;
    case PacketType::CS_ORDER_PICKUP:
        handlePickup(clientFd, j);
        break;
    case PacketType::CS_ORDER_READY:
        handleReady(clientFd, j);
        break;

    case PacketType::CS_ORDER_DONE:
        handleDone(clientFd, j);
        break;
    default:
        std::cerr << "[OrderHandler] 알 수 없는 패킷: " << packetType << "\n";
    }
}

void OrderHandler::handlePlaceOrder(int clientFd, const json &j)
{
    int customerId = j.value("customer_id", 0);
    int storeId = j.value("store_id", 0);
    std::string requestMsg = j.value("request_msg", "");
    json items = j.value("items", json::array());

    if (customerId == 0 || storeId == 0 || items.empty())
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_NOTI,
                                  {{"status", "fail"}, {"message", "필수 항목 누락"}});
        return;
    }

    int orderId = placeOrderUC_.execute(customerId, storeId, requestMsg, items);

    if (orderId == -1)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_NOTI,
                                  {{"status", "fail"}, {"message", "주문 생성 실패"}});
        return;
    }

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_NOTI,
                              {{"status", "success"},
                               {"order_id", orderId},
                               {"message", "주문이 접수되었습니다."}});
}

void OrderHandler::handleCancel(int clientFd, const json &j)
{
    int orderId = j.value("order_id", 0);
    int requesterId = j.value("customer_id", 0);

    if (orderId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                                  {{"status", "fail"}, {"message", "order_id 누락"}});
        return;
    }

    bool ok = cancelOrderUC_.execute(orderId, requesterId);

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                              {{"status", ok ? "success" : "fail"},
                               {"order_id", orderId},
                               {"message", ok ? "주문이 취소되었습니다." : "취소 실패"}});
}

void OrderHandler::handleAccept(int clientFd, const json &j)
{
    int orderId = j.value("order_id", 0);
    int estimatedTime = j.value("estimated_time", 0);
    bool accepted = j.value("accepted", false);

    if (orderId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                                  {{"status", "fail"}, {"message", "order_id 누락"}});
        return;
    }

    bool ok = acceptOrderUC_.execute(orderId, estimatedTime, accepted);

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                              {{"status", ok ? "success" : "fail"},
                               {"order_id", orderId},
                               {"message", accepted ? "주문을 수락했습니다." : "주문을 거절했습니다."}});
}

void OrderHandler::handlePickup(int clientFd, const json &j)
{
    int orderId = j.value("order_id", 0);
    if (orderId == 0)
        return;

    bool ok = updateStatusUC_.execute(orderId, "DELIVERING");

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                              {{"status", ok ? "success" : "fail"},
                               {"order_id", orderId},
                               {"message", "픽업 완료. 배달 시작"}});
}

void OrderHandler::handleDone(int clientFd, const json &j)
{
    int orderId = j.value("order_id", 0);
    if (orderId == 0)
        return;

    bool ok = updateStatusUC_.execute(orderId, "COMPLETED");

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                              {{"status", ok ? "success" : "fail"},
                               {"order_id", orderId},
                               {"message", "배달 완료"}});
}

void OrderHandler::handleReady(int clientFd, const json &j)
{
    int orderId = j.value("order_id", 0);
    if (orderId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                                  {{"status", "fail"}, {"message", "order_id 누락"}});
        return;
    }

    // 상태를 "DELIVERING" (또는 상황에 맞는 상태)로 업데이트
    bool ok = updateStatusUC_.execute(orderId, "READY");

    PacketHandler::sendPacket(clientFd, PacketType::SC_ORDER_STATUS,
                              {{"status", ok ? "success" : "fail"},
                               {"order_id", orderId},
                               {"message", ok ? "조리 완료 처리됨. 라이더를 호출합니다." : "상태 변경 실패"}});
}
#include "handlers/StoreHandler.hpp"
#include "PacketHandler.hpp"
#include "SessionManager.hpp"
#include <iostream>

void StoreHandler::handleStoreOpenReq(int clientFd, const json &j)
{
    // CSelectStatusDlg.cpp 에서 4910 이벤트를 받을 때 패킷 형태: { "type": 4910, "isOpen": 1 또는 0 }
    // 만약 클라이언트가 "isOpen" 필드를 안 보냈다면, 기본적으로 1 (오픈)로 간주하도록 방어적 프로그래밍.
    int isOpen = j.value("isOpen", 1); 

    // 세션에서 요청을 보낸 사용자의 아이디를 획득.
    std::string loginId = SessionManager::getInstance().getUserIdByFd(clientFd);
    if (loginId.empty()) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_OPEN_RES,
                                  {{"status", "fail"}, {"message", "로그인 상태가 아닙니다."}});
        return;
    }

    std::string outMsg;
    bool success = storeSvc_.toggleStoreStatus(loginId, isOpen, outMsg);

    if (success) {
        // 성공 시 4911 전송
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_OPEN_RES,
                                  {{"status", "success"}, {"message", outMsg}, {"isOpen", isOpen}});
    } else {
        // 실패 시 4911 전송
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_OPEN_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}

#include "handlers/StoreHandler.hpp"
#include "PacketHandler.hpp"
#include "SessionManager.hpp"
#include <iostream>

void StoreHandler::handleStoreOpenReq(int clientFd, const json &j)
{
    // { "type": 4910, "isOpen": 1, "storeId": 117 }
    int isOpen  = j.value("isOpen", 1);
    int storeId = j.value("storeId", 0);

    if (storeId == 0) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_OPEN_RES,
                                  {{"status", "fail"}, {"message", "가게 ID가 누락되었습니다."}});
        return;
    }

    // 세션에서 요청을 보낸 사용자의 아이디를 획득.
    std::string loginId = SessionManager::getInstance().getUserIdByFd(clientFd);
    if (loginId.empty()) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_OPEN_RES,
                                  {{"status", "fail"}, {"message", "로그인 상태가 아닙니다."}});
        return;
    }

    std::string outMsg;
    bool success = storeSvc_.toggleStoreStatus(loginId, storeId, isOpen, outMsg);

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

void StoreHandler::handleStoreRegister(int clientFd, const json &j)
{
    // CStoreRegisterDlg.cpp 등에서 4940 요청을 보낼 때 패킷 형태: 
    // { "type": 4940, "name": "...", "address": "...", "regNum": "...", "categoryId": 1 }
    
    std::string name     = j.value("name", "");
    std::string address  = j.value("address", "");
    std::string regNum   = j.value("regNum", "");
    int categoryId       = j.value("categoryId", 0);

    if (name.empty()) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_REGISTER_RES,
                                  {{"status", "fail"}, {"message", "가게 이름은 필수입니다."}});
        return;
    }

    // 세션에서 요청을 보낸 사용자의 아이디를 획득.
    std::string loginId = SessionManager::getInstance().getUserIdByFd(clientFd);
    if (loginId.empty()) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_REGISTER_RES,
                                  {{"status", "fail"}, {"message", "로그인 상태가 아닙니다."}});
        return;
    }

    std::string outMsg;
    bool success = storeSvc_.registerStore(loginId, name, address, regNum, categoryId, outMsg);

    if (success) {
        // 성공 시 4941 전송
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_REGISTER_RES,
                                  {{"status", "success"}, {"message", outMsg}});
    } else {
        // 실패 시 4941 전송
        PacketHandler::sendPacket(clientFd, PacketType::SC_STORE_REGISTER_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}

void StoreHandler::handleOwnerStoreList(int clientFd, const json &j)
{
    std::string loginId = SessionManager::getInstance().getUserIdByFd(clientFd);
    if (loginId.empty()) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_OWNER_STORE_LIST_RES,
                                  {{"status", "fail"}, {"message", "로그인 상태가 아닙니다."}});
        return;
    }

    json stores = storeSvc_.getOwnerStores(loginId);
    PacketHandler::sendPacket(clientFd, PacketType::SC_OWNER_STORE_LIST_RES,
                              {{"status", "success"}, {"stores", stores}});
}

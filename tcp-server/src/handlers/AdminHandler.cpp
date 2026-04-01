#include "handlers/AdminHandler.hpp"
#include "PacketHandler.hpp"
#include <iostream>

// 생성자에서 UseCase들을 초기화해줍니다.
AdminHandler::AdminHandler()
    : adminSvc_(), searchUserUC_(adminSvc_), searchHistoryUC_(adminSvc_)
{
}

void AdminHandler::handleSearchId(int clientFd, const json &j)
{
    // [수정] login_id 키가 있으면 login_id 로 조회
    if (j.contains("login_id") && j["login_id"].is_string())
    {
        std::string loginId = j.value("login_id", "");
        if (loginId.empty())
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                      {{"status", "fail"}, {"message", "login_id가 비어있습니다."}});
            return;
        }

        json userInfo = adminSvc_.getUserByLoginId(loginId); // [수정] 신규 함수 호출

        if (userInfo.empty())
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                      {{"status", "fail"}, {"message", "해당 유저를 찾을 수 없습니다."}});
        }
        else
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                      {{"status", "success"}, {"data", userInfo}});
        }
        return;
    }

    // 기존: target_id 숫자로 조회 (변경 없음)
    int targetId = j.value("target_id", 0);

    if (targetId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                  {{"status", "fail"}, {"message", "검색할 유저 ID(target_id)가 누락되었습니다."}});
        return;
    }

    json userInfo = searchUserUC_.execute(targetId);

    if (userInfo.empty())
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                  {{"status", "fail"}, {"message", "해당 유저를 찾을 수 없습니다."}});
    }
    else
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                  {{"status", "success"}, {"data", userInfo}});
    }
}

void AdminHandler::handleSearchHistory(int clientFd, const json &j)
{
    if (j.contains("login_id") && j["login_id"].is_string())
    {
        std::string loginId = j.value("login_id", "");
        if (loginId.empty())
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_HISTORY_RES,
                                      {{"status", "fail"}, {"message", "login_id가 비어있습니다."}});
            return;
        }

        json history = searchHistoryUC_.execute(loginId);
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_HISTORY_RES,
                                  {{"status", "success"}, {"data", history}});
        return;
    }

    int targetId = j.value("target_id", 0);
    if (targetId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_HISTORY_RES,
                                  {{"status", "fail"}, {"message", "검색할 유저 ID(target_id)가 누락되었습니다."}});
        return;
    }

    json history = searchHistoryUC_.execute(targetId);
    PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_HISTORY_RES,
                              {{"status", "success"}, {"data", history}});
}

void AdminHandler::handleRefund(int clientFd, const json &j)
{
    std::string targetIdStr = j.value("targetId", "");
    std::string amountStr = j.value("amount", "0");

    if (targetIdStr.empty())
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_REFUND_RES,
                                  {{"status", "fail"}, {"message", "targetId 누락"}});
        return;
    }

    int orderId = 0;
    try
    {
        orderId = std::stoi(targetIdStr);
    }
    catch (...)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_REFUND_RES,
                                  {{"status", "fail"}, {"message", "targetId 형식이 올바르지 않습니다."}});
        return;
    }

    // 관리자 ID 찾기
    std::string email = SessionManager::getInstance().getUserIdByFd(clientFd);
    int adminId = adminSvc_.getAdminIdByEmail(email);

    std::string outMsg;
    int customerId = 0;
    int storeId = 0;
    bool success = adminSvc_.processRefund(orderId, amountStr, adminId, outMsg, customerId, storeId);

    if (success)
    {
        // 알림 전송 (고객에게)
        notifySvc_.notifyCustomer(customerId, orderId, "CANCELLED");

        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_REFUND_RES,
                                  {{"status", "success"}, {"message", outMsg}});
    }
    else
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_REFUND_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}

void AdminHandler::handleBanUser(int clientFd, const json &j)
{
    std::string targetIdStr = j.value("targetId", "");
    std::string reason = j.value("reason", "");

    if (targetIdStr.empty())
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_BAN_USER_RES,
                                  {{"status", "fail"}, {"message", "targetId 누락"}});
        return;
    }

    int memberId = 0;
    try
    {
        memberId = std::stoi(targetIdStr);
    }
    catch (...)
    {
        // 숫자가 아닌 경우 login_id로 간주하여 조회 시도
        json userInfo = adminSvc_.getUserByLoginId(targetIdStr);
        if (!userInfo.empty() && userInfo.contains("RES_member_id"))
        {
            memberId = userInfo["RES_member_id"].get<int>();
        }
        else
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_BAN_USER_RES,
                                      {{"status", "fail"}, {"message", "존재하지 않는 로그인 아이디입니다: " + targetIdStr}});
            return;
        }
    }

    // 관리자 ID 찾기
    std::string email = SessionManager::getInstance().getUserIdByFd(clientFd);
    int adminId = adminSvc_.getAdminIdByEmail(email);

    std::string outMsg;
    bool success = adminSvc_.processBanUser(memberId, reason, adminId, outMsg);

    if (success)
    {
        // 차단 알림 및 연결 해제
        notifySvc_.notifyBan(memberId, reason);

        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_BAN_USER_RES,
                                  {{"status", "success"}, {"message", outMsg}});
    }
    else
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_BAN_USER_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}

void AdminHandler::handleClearBanUser(int clientFd, const json &j)
{
    std::string targetIdStr = j.value("targetId", "");
    std::string reason = j.value("reason", "차단 해제 처리"); // 해제 사유

    if (targetIdStr.empty())
    {
        // [참고] 패킷 타입 이름은 프로젝트 설정에 맞게 변경하세요 (예: SC_ADMIN_CLEAR_BAN_RES)
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_CLEAR_BAN_USER_RES,
                                  {{"status", "fail"}, {"message", "targetId 누락"}});
        return;
    }

    int memberId = 0;
    try
    {
        memberId = std::stoi(targetIdStr);
    }
    catch (...)
    {
        // 숫자가 아닌 경우 login_id로 간주하여 조회 시도
        json userInfo = adminSvc_.getUserByLoginId(targetIdStr);
        if (!userInfo.empty() && userInfo.contains("RES_member_id"))
        {
            memberId = userInfo["RES_member_id"].get<int>();
        }
        else
        {
            PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_CLEAR_BAN_USER_RES,
                                      {{"status", "fail"}, {"message", "존재하지 않는 로그인 아이디입니다: " + targetIdStr}});
            return;
        }
    }

    // 관리자 ID 찾기
    std::string email = SessionManager::getInstance().getUserIdByFd(clientFd);
    int adminId = adminSvc_.getAdminIdByEmail(email);

    std::string outMsg;
    // [수정] 차단 해제용 신규 함수 호출
    bool success = adminSvc_.processClearBanUser(memberId, reason, adminId, outMsg);

    if (success)
    {
        // (선택 사항) 차단 해제 알림 전송이 필요하다면 여기에 추가
        // notifySvc_.notifyClearBan(memberId, reason);

        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_CLEAR_BAN_USER_RES,
                                  {{"status", "success"}, {"message", outMsg}});
    }
    else
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_CLEAR_BAN_USER_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}
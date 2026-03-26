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

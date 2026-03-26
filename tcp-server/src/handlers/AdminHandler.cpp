#include "handlers/AdminHandler.hpp"
#include "PacketHandler.hpp" // 패킷 전송을 위해 필요
#include <iostream>

// 생성자에서 UseCase들을 초기화해줍니다.
AdminHandler::AdminHandler()
    : adminSvc_(), searchUserUC_(adminSvc_)
{
}

void AdminHandler::handleSearchId(int clientFd, const json &j)
{
    // 1. 파라미터 추출 (클라이언트가 "target_id"를 보냈다고 가정)
    int targetId = j.value("target_id", 0);

    if (targetId == 0)
    {
        PacketHandler::sendPacket(clientFd, PacketType::SC_ADMIN_SEARCH_ID_RES,
                                  {{"status", "fail"}, {"message", "검색할 유저 ID(target_id)가 누락되었습니다."}});
        return;
    }

    // 2. UseCase 호출하여 실제 비즈니스 로직 실행
    json userInfo = searchUserUC_.execute(targetId);

    // 3. 결과에 따라 클라이언트로 응답 패킷 전송
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
    // 아직 히스토리 검색 기능은 구현되지 않았으므로 임시로 빈 함수 처리
    std::cout << "[AdminHandler] handleSearchHistory가 호출되었습니다. (기능 미구현)\n";
}
#include "handlers/ChatHandler.hpp"

#include <iostream>
#include <vector>

#include "SessionManager.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"

void ChatHandler::handleChat(int client_fd, const nlohmann::json &j) const
{
    std::string senderId = j.value("senderId", "");
    std::string msg = j.value("message", "");
    int senderType = j.value("clientType", -1); // 기본값을 -1로 두어 실수를 방지합니다.

    std::cout << "[채팅 수신] " << senderId << "(Type: " << senderType << "): " << msg << "\n";

    // 🌟 1. 고객(1), 사장(2), 라이더(3)가 보낸 경우 -> 관리자(0)에게 전달 : enum을 int로 바꾸기
    if (senderType == static_cast<int>(ClientType::CUSTOMER) || senderType == static_cast<int>(ClientType::OWNER) || senderType == static_cast<int>(ClientType::RIDER))
    {
        // SessionManager::getAdminFds() 내부 로직이 '0'번을 찾는지 반드시 확인하세요!
        std::vector<int> admins = SessionManager::getInstance().getAdminFds();

        if (admins.empty())
        {
            std::cout << "[경고] 접속 중인 관리자(Type 0)가 없어 메시지 증발.\n";
            return;
        }

        nlohmann::json notiPayload = {
            {"senderId", senderId},
            {"message", msg},
            {"clientType", senderType},
            {"info", "현장 문의 (고객/가게/라이더)"}};

        for (int adminFd : admins)
        {
            PacketHandler::sendPacket(adminFd, PacketType::SC_CHAT_NOTI, notiPayload);
        }
        std::cout << "[라우팅] 현장 -> 관리자(" << admins.size() << "명) 전달 완료.\n";
    }
    // 🌟 2. 관리자(0)가 보낸 경우 -> targetId 유저에게 전달
    else if (senderType == static_cast<int>(ClientType::ADMIN))
    {
        std::string targetId = j.value("targetId", "");
        int targetFd = SessionManager::getInstance().getFdByUserId(targetId);

        if (targetFd != -1)
        {
            nlohmann::json notiPayload = {
                {"senderId", "ADMIN"},
                {"message", msg}};
            PacketHandler::sendPacket(targetFd, PacketType::SC_CHAT_NOTI, notiPayload);
            std::cout << "[라우팅] 관리자 -> " << targetId << " 전달 완료.\n";
        }
        else
        {
            std::cout << "[경고] 타겟 유저(" << targetId << ") 오프라인 상태.\n";
        }
    }
    else
    {
        std::cerr << "[에러] 알 수 없는 clientType의 채팅 요청: " << senderType << "\n";
    }

    (void)client_fd;
}
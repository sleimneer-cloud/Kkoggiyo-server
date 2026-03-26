#include "router/MessageRouter.hpp"
#include "protocol.hpp"

#include <iostream>

#include "json.hpp"
#include "handlers/AuthHandler.hpp"
#include "handlers/ChatHandler.hpp"
#include "handlers/OrderHandler.hpp"
#include "handlers/AdminHandler.hpp"

using json = nlohmann::json;

// 커넥션 메니저에서 넘어온 데이터를 패킷 타입에 따라 알맞은 핸들러로 전달하는 역할
void MessageRouter::route(int client_fd, const std::string &jsonStr) const
{
    if (jsonStr.empty())
    {
        std::cerr << "[MessageRouter] 빈 jsonStr 수신, 무시\n";
        return;
    }

    try
    {
        json j = json::parse(jsonStr);
        int msgType = j.value("type", -1);

        switch (static_cast<PacketType>(msgType))
        {
        case PacketType::CS_LOGIN_REQ:
            authHandler_.handleLogin(client_fd, j);
            break;
        case PacketType::CS_REGISTER_REQ:
            authHandler_.handleRegister(client_fd, j);
            break;
        case PacketType::CS_CHAT_REQ:
            chatHandler_.handleChat(client_fd, j);
            break;
        case PacketType::CS_ADMIN_SEARCH_ID:
            adminHandler_.handleSearchId(client_fd, j);
            break;
        case PacketType::CS_ADMIN_SEARCH_HISTORY:
            adminHandler_.handleSearchHistory(client_fd, j);
            break;

        // ── 주문 관련 ─────────────────────────────────── ← 추가
        case PacketType::CS_ORDER_REQ:    // 300
        case PacketType::CS_ORDER_CANCEL: // 304
        case PacketType::CS_ORDER_ACCEPT: // 305
        case PacketType::CS_ORDER_PICKUP: // 306
        case PacketType::CS_ORDER_DONE:   // 307
            OrderHandler{}.handle(client_fd, msgType, j);
            break;

        default:
            std::cout << "[경고] 알 수 없는 패킷 타입: " << msgType << "\n";
            break;
        }
    }
    catch (const json::parse_error &e)
    {
        std::cerr << "[MessageRouter] JSON 파싱 실패 (FD: " << client_fd
                  << "): " << e.what() << '\n';
    }
    catch (const std::exception &e) // ← 이걸 추가
    {
        std::cerr << "[MessageRouter] 처리 중 예외 (FD: " << client_fd
                  << "): " << e.what() << '\n';
    }
    catch (...) // ← 알 수 없는 예외까지 대비
    {
        std::cerr << "[MessageRouter] 알 수 없는 예외 발생 (FD: " << client_fd << ")\n";
    }
}
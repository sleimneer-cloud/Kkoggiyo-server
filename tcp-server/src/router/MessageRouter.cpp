#include "router/MessageRouter.hpp"
#include "protocol.hpp"

#include <iostream>

#include "json.hpp"
#include "handlers/AuthHandler.hpp"
#include "handlers/ChatHandler.hpp"
#include "handlers/OrderHandler.hpp"
#include "handlers/AdminHandler.hpp"
#include "handlers/StoreHandler.hpp"

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
        case PacketType::USER_REGISTER_REQ:
            authHandler_.handleRegister(client_fd, j);
            break;
        case PacketType::USER_VERIFY_REQ:
            authHandler_.handleVerify(client_fd, j);
            break;
        case PacketType::USER_LOGIN_REQ:
            authHandler_.handleLogin(client_fd, j);
            break;
        case PacketType::USER_NAME_ALT_REQ:
            authHandler_.handleNameChange(client_fd, j);
            break;
        case PacketType::USER_WITHDRAW_PW_REQ:
            authHandler_.handleWithdrawVerify(client_fd, j);
            break;
        case PacketType::USER_WITHDRAW_REQ:
            authHandler_.handleWithdraw(client_fd, j);
            break;
        case PacketType::USER_ID_CHECK_REQ:
            authHandler_.handleIdCheck(client_fd, j);
            break;
        case PacketType::USER_ADDR_ADD_REQ:
            authHandler_.handleAddAddress(client_fd, j);
            break;
        case PacketType::USER_ADDR_LIST_REQ:
            authHandler_.handleGetAddresses(client_fd, j);
            break;
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
        case PacketType::CS_ADMIN_REFUND_REQ:
            adminHandler_.handleRefund(client_fd, j);
            break;
        case PacketType::CS_ADMIN_BAN_USER:
            adminHandler_.handleBanUser(client_fd, j);
            break;
        case PacketType::CS_ADMIN_CLEAR_BAN_USER:
            adminHandler_.handleClearBanUser(client_fd, j);
            break;
        case PacketType::CS_STORE_OPEN_REQ: // 4910
            storeHandler_.handleStoreOpenReq(client_fd, j);
            break;
        case PacketType::CS_STORE_REGISTER_REQ: // 4940
            storeHandler_.handleStoreRegister(client_fd, j);
            break;
        case PacketType::CS_OWNER_STORE_LIST_REQ: // 4960
            storeHandler_.handleOwnerStoreList(client_fd, j);
            break;
        case PacketType::CS_MENU_REGISTER_REQ: // 4950
            menuHandler_.handleMenuRegister(client_fd, j);
            break;
        case PacketType::USER_STORE_LIST_REQ:
            clientHandler_.handleGetStores(client_fd, j);
            break;
        case PacketType::USER_MENU_LIST_REQ:
            clientHandler_.handleGetMenus(client_fd, j);
            break;

                // ── 주문 관련 ─────────────────────────────────── ← 추가
        case PacketType::CS_ORDER_REQ:         // 300
        case PacketType::CS_ORDER_CANCEL:      // 304
        case PacketType::CS_ORDER_ACCEPT:      // 305
        case PacketType::CS_ORDER_PICKUP:      // 306
        case PacketType::CS_ORDER_READY:       // 307
        case PacketType::CS_ORDER_DONE:        // 308
        case PacketType::CS_OWNER_ORDER_LIST_REQ: // 310
        case PacketType::CS_RIDER_ORDER_LIST_REQ: // 320
        case PacketType::CS_ORDER_ASSIGN_REQ:    // 322
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
#include "handlers/ClientHandler.hpp"
#include "services/ClientService.hpp"
#include "PacketHandler.hpp"
#include <iostream>
#include "protocol.hpp"

ClientHandler::ClientHandler() {}

// 가게 목록 조회
void ClientHandler::handleGetStores(int client_fd, const nlohmann::json &j) const
{
    ClientService clientSvc;
    auto stores = clientSvc.processGetStores(j);

    nlohmann::json res;
    res["status"] = "success";
    res["stores"] = stores;
    PacketHandler::sendPacket(client_fd, PacketType::USER_STORE_LIST_RES, res);
}

// 메뉴 목록 조회
void ClientHandler::handleGetMenus(int client_fd, const nlohmann::json &j) const
{
    ClientService clientSvc;
    auto menus = clientSvc.processGetMenus(j);

    nlohmann::json res;
    res["status"] = "success";
    res["menus"] = menus;
    PacketHandler::sendPacket(client_fd, PacketType::USER_MENU_LIST_RES, res);
}
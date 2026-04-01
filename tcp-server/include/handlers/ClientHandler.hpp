#pragma once

#include "json.hpp"

class ClientHandler
{
public:
    ClientHandler();
    ~ClientHandler() = default;

    // 가게 목록 조회
    void handleGetStores(int client_fd, const nlohmann::json &j) const;

    // 메뉴 목록 조회
    void handleGetMenus(int client_fd, const nlohmann::json &j) const;
};
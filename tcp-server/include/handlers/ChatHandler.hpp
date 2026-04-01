#pragma once

#include "json.hpp"

// 관리자 중재 채팅 처리
class ChatHandler
{
public:
    void handleChat(int client_fd, const nlohmann::json &j) const;
};


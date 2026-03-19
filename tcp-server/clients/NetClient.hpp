#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "protocol.hpp"
#include "json.hpp"

class NetClient
{
public:
    NetClient() : sock_(-1) {}
    ~NetClient() { close(); }

    bool connect(const std::string &host, uint16_t port);
    bool sendPacket(int32_t type, const nlohmann::json &payload);
    std::pair<int32_t, nlohmann::json> recvPacket(); // 실패 시 (-1, null)

    void close();
    bool isConnected() const { return sock_ >= 0; }

private:
    int sock_;
};

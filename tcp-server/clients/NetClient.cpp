#include "NetClient.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

bool NetClient::connect(const std::string &host, uint16_t port)
{
    sock_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0)
    {
        std::cerr << "[NetClient] socket() 실패\n";
        return false;
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
    {
        std::cerr << "[NetClient] 잘못된 주소: " << host << "\n";
        ::close(sock_);
        sock_ = -1;
        return false;
    }

    if (::connect(sock_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "[NetClient] connect() 실패. 서버가 켜져 있는지 확인하세요.\n";
        ::close(sock_);
        sock_ = -1;
        return false;
    }
    return true;
}

bool NetClient::sendPacket(int32_t type, const nlohmann::json &payload)
{
    if (sock_ < 0)
        return false;

    nlohmann::json msg = payload;
    msg["type"] = type;
    std::string body = msg.dump();
    int32_t len = htonl(static_cast<int32_t>(body.size())); // ← htonl 추가

    ssize_t n = ::send(sock_, &len, sizeof(int32_t), 0);
    if (n != static_cast<ssize_t>(sizeof(int32_t)))
        return false;
    n = ::send(sock_, body.data(), body.size(), 0);
    if (n != static_cast<ssize_t>(body.size()))
        return false;
    return true;
}

std::pair<int32_t, nlohmann::json> NetClient::recvPacket()
{
    if (sock_ < 0)
        return {-1, nullptr};

    int32_t payloadSize;
    ssize_t n = ::recv(sock_, &payloadSize, sizeof(int32_t), 0);
    if (n != static_cast<ssize_t>(sizeof(int32_t)))
        return {-1, nullptr};

    payloadSize = ntohl(static_cast<uint32_t>(payloadSize)); // ← ntohl 추가

    if (payloadSize < 0 || payloadSize > 1024 * 1024)
        return {-1, nullptr};

    std::string body(static_cast<size_t>(payloadSize), '\0');
    size_t got = 0;
    while (got < static_cast<size_t>(payloadSize))
    {
        n = ::recv(sock_, &body[got], static_cast<size_t>(payloadSize) - got, 0);
        if (n <= 0)
            return {-1, nullptr};
        got += static_cast<size_t>(n);
    }

    try
    {
        auto j = nlohmann::json::parse(body);
        int32_t type = j.value("type", -1);
        return {type, j};
    }
    catch (...)
    {
        return {-1, nullptr};
    }
}

void NetClient::close()
{
    if (sock_ >= 0)
    {
        ::close(sock_);
        sock_ = -1;
    }
}

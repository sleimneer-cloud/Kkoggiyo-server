#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <sys/epoll.h>

#include "net/ConnectionState.hpp"
#include "router/MessageRouter.hpp"
#include "thread/WorkQueue.hpp"

// TCP accept/epoll/read + 패킷 프레이밍을 전담하는 네트워크 코어
class ConnectionManager
{
public:
    ConnectionManager(int port, MessageRouter router);
    ~ConnectionManager();

    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager &operator=(const ConnectionManager &) = delete;

    void run();

private:
    void setNonBlocking(int sock);
    void acceptClient();
    void onReadable(int client_fd);
    void disconnectClient(int client_fd);
    void extractAndRoutePackets(int client_fd);

    int port_;
    int server_fd_{-1};
    int epoll_fd_{-1};
    MessageRouter router_;
    std::unordered_map<int, ConnectionState> connections_;
    WorkQueue workQueue_; // 패킷 처리 작업을 위한 워크 큐

    static constexpr int MAX_EVENTS = 100;
    static constexpr size_t READ_CHUNK_SIZE = 4096;
    static constexpr size_t MAX_PAYLOAD_SIZE = 1024 * 1024;
};

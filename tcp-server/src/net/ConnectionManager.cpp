#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>

#include "PacketHandler.hpp"
#include "net/ConnectionManager.hpp"
#include "SessionManager.hpp"

ConnectionManager::ConnectionManager(int port, MessageRouter router)
    : port_(port), router_(std::move(router)), workQueue_(8, [this](WorkItem item) { // Worker 8개
          router_.route(item.client_fd, item.jsonStr);
      })

{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd_, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1)
        throw std::runtime_error("bind() failed");

    if (listen(server_fd_, 10) == -1)
        throw std::runtime_error("listen() failed");

    setNonBlocking(server_fd_);

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1)
        throw std::runtime_error("epoll_create1() failed");

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) == -1)
        throw std::runtime_error("epoll_ctl(ADD server) failed");
}

ConnectionManager::~ConnectionManager()
{
    if (server_fd_ != -1)
        close(server_fd_);
    if (epoll_fd_ != -1)
        close(epoll_fd_);
}

void ConnectionManager::setNonBlocking(int sock)
{
    int opts = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

void ConnectionManager::run()
{
    std::cout << "[서버 시작] 포트 " << port_ << "에서 클라이언트 접속 대기 중...\n";

    epoll_event events[MAX_EVENTS];
    while (true)
    {
        int event_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
        for (int i = 0; i < event_count; ++i)
        {
            int fd = events[i].data.fd;
            if (fd == server_fd_)
                acceptClient();
            else
                onReadable(fd);
        }
    }
}

void ConnectionManager::acceptClient()
{
    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            return;
        }

        setNonBlocking(client_fd);

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

        connections_.emplace(client_fd, ConnectionState{});
        std::cout << "[알림] 새 클라이언트 접속! FD: " << client_fd << "\n";
    }
}

void ConnectionManager::onReadable(int client_fd)
{
    auto it = connections_.find(client_fd);
    if (it == connections_.end())
    {
        disconnectClient(client_fd);
        return;
    }

    auto &buf = it->second.recvBuffer;

    while (true)
    {
        uint8_t tmp[READ_CHUNK_SIZE];
        ssize_t bytes_read = read(client_fd, tmp, sizeof(tmp)); // 여기서 클라에서 보낸 데이터 읽음

        if (bytes_read == 0)
        {
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }
        if (bytes_read < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }

        buf.insert(buf.end(), tmp, tmp + bytes_read); // 읽은 데이터는 클라별 recvBuffer에 누적
    }

    extractAndRoutePackets(client_fd);
}

void ConnectionManager::disconnectClient(int client_fd)
{
    std::cout << "[알림] 클라이언트 접속 종료. FD: " << client_fd << "\n";
    SessionManager::getInstance().removeSession(client_fd);
    PacketHandler::removeFd(client_fd); // PacketHandler에서 해당 FD 제거 (세션 종료 시 필요)
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
    close(client_fd);
}

void ConnectionManager::extractAndRoutePackets(int client_fd) // 클라에서 받은 데이터가 담긴 버퍼에서 패킷 단위로 잘라서 MessageRouter에 전달
{
    auto it = connections_.find(client_fd);
    if (it == connections_.end())
        return;

    auto &buf = it->second.recvBuffer;

    while (true)
    {
        if (buf.size() < sizeof(int32_t))
            return;

        int32_t payloadSize;
        std::memcpy(&payloadSize, buf.data(), sizeof(int32_t));

        if (payloadSize < 0 || static_cast<size_t>(payloadSize) > MAX_PAYLOAD_SIZE)
        {
            std::cout << "[경고] 비정상 payloadSize: " << payloadSize << " FD: " << client_fd << " 연결 종료\n";
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }

        if (payloadSize == 0)
        {
            buf.erase(buf.begin(), buf.begin() + static_cast<long>(sizeof(int32_t)));
            continue;
        }

        size_t frameSize = sizeof(int32_t) + static_cast<size_t>(payloadSize);
        if (buf.size() < frameSize)
            return;

        std::string jsonStr(reinterpret_cast<const char *>(buf.data() + sizeof(int32_t)),
                            static_cast<size_t>(payloadSize));

        workQueue_.push({client_fd, jsonStr});
        ; // Worker 스레드에 로직 처리 위임

        buf.erase(buf.begin(), buf.begin() + static_cast<long>(frameSize)); // 처리한 패킷만큼 버퍼에서 제거
    }
}

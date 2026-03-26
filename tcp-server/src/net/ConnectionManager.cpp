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

ConnectionManager::ConnectionManager(int port, MessageRouter router)                 // default 생성자에서 서버 소켓 생성, 바인딩, 리슨까지 모두 처리. MessageRouter는 라우팅 로직이 담긴 객체로, ConnectionManager가 클라이언트로부터 받은 패킷을 이 객체에 전달하여 처리하도록 함
    : port_(port), router_(std::move(router)), workQueue_(8, [this](WorkItem item) { // Worker 8개
          router_.route(item.client_fd, item.jsonStr);
      }) // default 생성자에서 서버 소켓 생성, 바인딩, 리슨까지 모두 처리.
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 소켓 옵션 설정 (주소 재사용)
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)); // 소켓 옵션 설정 (포트 재사용)

    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr)); // 서버 주소 구조체 초기화
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);            // 모든 인터페이스에서 연결 수락
    server_addr.sin_port = htons(static_cast<uint16_t>(port_)); // 포트 번호 설정

    if (bind(server_fd_, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) == -1) // 소켓에 주소 바인딩
        throw std::runtime_error("bind() failed");

    if (listen(server_fd_, 10) == -1) // 소켓을 수신 대기 상태로 설정
        throw std::runtime_error("listen() failed");

    setNonBlocking(server_fd_); // 서버 소켓을 논블로킹 모드로 설정

    epoll_fd_ = epoll_create1(0); // epoll 인스턴스 생성
    if (epoll_fd_ == -1)
        throw std::runtime_error("epoll_create1() failed");

    epoll_event ev;
    ev.events = EPOLLIN;                                            // 서버 소켓에서 읽기 이벤트 감지
    ev.data.fd = server_fd_;                                        // 이벤트 데이터에 서버 소켓 파일 디스크립터 저장
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) == -1) // epoll 인스턴스에 서버 소켓 등록
        throw std::runtime_error("epoll_ctl(ADD server) failed");   // epoll_ctl() 함수는 epoll 인스턴스에 파일 디스크립터를 등록, 수정, 삭제하는 함수.
                                                                    // 여기서는 서버 소켓을 epoll 인스턴스에 등록하여 읽기 이벤트를 감지하도록 설정합니다.
}

ConnectionManager::~ConnectionManager() // 소멸자에서는 서버 소켓과 epoll 인스턴스의 파일 디스크립터를 닫아 리소스를 해제합니다.
{
    if (server_fd_ != -1)
        close(server_fd_);
    if (epoll_fd_ != -1)
        close(epoll_fd_);
}

void ConnectionManager::setNonBlocking(int sock) // 소켓을 논블로킹 모드로 설정하는 함수입니다.
// fcntl() 함수를 사용하여 소켓의 파일 상태 플래그를 가져오고, O_NONBLOCK 플래그를 추가하여 다시 설정합니다.
// 이렇게 하면 소켓이 논블로킹 모드로 동작하게 되어, read()나 accept() 같은 함수가 즉시 반환하도록 합니다.
{
    int opts = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, opts | O_NONBLOCK);
}

void ConnectionManager::run() // 서버의 메인 루프입니다. epoll_wait() 함수를 사용하여 이벤트를 기다리고, 이벤트가 발생하면 해당 파일 디스크립터에 대해 적절한 처리를 수행합니다.
{
    std::cout << "[서버 시작] 포트 " << port_ << "에서 클라이언트 접속 대기 중...\n";

    epoll_event events[MAX_EVENTS];
    while (true)
    {
        int event_count = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1); // epoll_wait() 함수는 epoll 인스턴스에서 이벤트가 발생할 때까지 대기하며,
        // 발생한 이벤트의 수를 반환합니다. -1은 무한 대기를 의미합니다.
        for (int i = 0; i < event_count; ++i) // for 루프에서는 발생한 이벤트를 순회하며, 서버 소켓에서 이벤트가 발생하면 acceptClient() 함수를 호출하여 새로운 클라이언트를 수락하고,
                                              // 그렇지 않으면 onReadable() 함수를 호출하여 해당 클라이언트로부터 데이터를 읽어 처리합니다.
        {
            int fd = events[i].data.fd;
            if (fd == server_fd_)
                acceptClient();
            else
                onReadable(fd);
        }
    }
}

void ConnectionManager::acceptClient() // 새로운 클라이언트를 수락하는 함수입니다.
// accept() 함수를 사용하여 클라이언트의 연결을 수락하고, 새로 생성된 클라이언트 소켓을 논블로킹 모드로 설정합니다.
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
        ev.events = EPOLLIN | EPOLLET;                       // 읽기 이벤트와 엣지 트리거 모드 설정
        ev.data.fd = client_fd;                              // 이벤트 데이터에 클라이언트 소켓 파일 디스크립터 저장
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev); // epoll 인스턴스에 클라이언트 소켓 등록

        connections_.emplace(client_fd, ConnectionState{}); // 클라이언트 소켓을 connections_ 맵에 추가하여 관리
        // connections_ 맵은 클라이언트 소켓 파일 디스크립터를 키로, ConnectionState 객체를 값으로 저장하는 자료구조입니다.
        // ConnectionState는 각 클라이언트의 상태를 관리하는 구조체로, 예를 들어 수신 버퍼 등을 포함할 수 있습니다.
        std::cout << "[알림] 새 클라이언트 접속! FD: " << client_fd << "\n";
    }
}

void ConnectionManager::onReadable(int client_fd) // 클라이언트로부터 데이터를 읽어 처리하는 함수입니다.
// 먼저 connections_ 맵에서 해당 클라이언트의 상태를 가져오고, read() 함수를 사용하여 데이터를 읽습니다.
{
    auto it = connections_.find(client_fd); // 클라이언트 소켓이 connections_ 맵에 존재하는지 확인합니다.
    // 존재하지 않으면 해당 클라이언트는 이미 연결이 종료된 상태이므로 disconnectClient() 함수를 호출하여 정리합니다.
    if (it == connections_.end())
    {
        disconnectClient(client_fd);
        return;
    }

    auto &buf = it->second.recvBuffer; // 클라이언트 소켓에 대한 수신 버퍼를 가져옵니다. 이 버퍼는 클라이언트로부터 받은 데이터를 누적하여 저장하는 역할을 합니다.

    while (true)
    {
        uint8_t tmp[READ_CHUNK_SIZE];                           // 데이터를 읽을 임시 버퍼입니다. READ_CHUNK_SIZE는 한 번에 읽을 최대 데이터 크기를 정의하는 상수입니다.
        ssize_t bytes_read = read(client_fd, tmp, sizeof(tmp)); // 여기서 클라에서 보낸 데이터 읽음

        if (bytes_read == 0) // 클라이언트가 연결을 종료한 경우
        {
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }
        if (bytes_read < 0) // 읽기 오류가 발생한 경우
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }

        buf.insert(buf.end(), tmp, tmp + bytes_read); // 읽은 데이터는 클라별 recvBuffer에 누적
                                                      // 🚨 [추가할 부분] 무한 버퍼 증식 방어 (Hard Limit)
        // 수신 버퍼 자체가 너무 커지면(예: MAX_PAYLOAD_SIZE의 2배),
        // 악의적인 공격(Slowloris 등)이나 비정상 클라이언트로 간주하고 즉시 추방!
        if (buf.size() > MAX_PAYLOAD_SIZE * 2)
        {
            std::cerr << "[보안 경고] 수신 버퍼 한계치 초과 공격 감지! FD: " << client_fd << "\n";
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }
    }

    extractAndRoutePackets(client_fd); // 클라이언트로부터 받은 데이터가 담긴 버퍼에서 패킷 단위로 잘라서 MessageRouter에 전달하는 함수입니다.
    // 이 함수는 recvBuffer에서 패킷을 추출하여 MessageRouter의 route() 메서드에 전달하여 처리하도록 합니다.
}

void ConnectionManager::disconnectClient(int client_fd)
{
    std::cout << "[알림] 클라이언트 접속 종료. FD: " << client_fd << "\n";
    SessionManager::getInstance().removeSession(client_fd); // SessionManager에서 해당 세션 제거 (세션 종료 시 필요)
    PacketHandler::removeFd(client_fd);                     // PacketHandler에서 해당 FD 제거 (세션 종료 시 필요)
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);   // epoll 인스턴스에서 클라이언트 소켓 제거
    close(client_fd);                                       // 클라이언트 소켓 닫기
}

void ConnectionManager::extractAndRoutePackets(int client_fd) // 클라에서 받은 데이터가 담긴 버퍼에서 패킷 단위로 잘라서 MessageRouter에 전달
{
    auto it = connections_.find(client_fd); // 클라이언트 소켓이 connections_ 맵에 존재하는지 확인합니다.
    if (it == connections_.end())           // 존재하지 않으면 해당 클라이언트는 이미 연결이 종료된 상태이므로 disconnectClient() 함수를 호출하여 정리합니다.
        return;

    auto &buf = it->second.recvBuffer; // 클라이언트 소켓에 대한 수신 버퍼를 가져옵니다. 이 버퍼는 클라이언트로부터 받은 데이터를 누적하여 저장하는 역할을 합니다.

    while (true)
    {
        if (buf.size() < sizeof(int32_t)) // 패킷의 헤더(4바이트)보다 버퍼에 데이터가 적으면 패킷이 완성되지 않은 상태이므로
            return;

        int32_t payloadSize;                                    // 패킷의 헤더에서 payload 크기를 읽어옵니다. 패킷의 헤더는 4바이트로, 이 부분에서 payload의 크기를 나타내는 정수값이 저장되어 있습니다.
        std::memcpy(&payloadSize, buf.data(), sizeof(int32_t)); // memcpy() 함수를 사용하여 버퍼에서 payload 크기를 읽어옵니다.
        // buf.data()는 버퍼의 시작 주소를 반환하며, sizeof(int32_t)만큼의 데이터를 payloadSize 변수에 복사합니다.

        payloadSize = ntohl(static_cast<uint32_t>(payloadSize)); // ntohl() 함수를 사용하여 네트워크 바이트 순서로 저장된 payload 크기를 호스트 바이트 순서로 변환합니다.

        if (payloadSize < 0 || static_cast<size_t>(payloadSize) > MAX_PAYLOAD_SIZE) // payload 크기가 음수이거나 최대 허용 크기를 초과하는 경우,

        // 비정상적인 패킷으로 간주하여 해당 클라이언트와의 연결을 종료합니다.
        {
            std::cout << "[경고] 비정상 payloadSize: " << payloadSize << " FD: " << client_fd << " 연결 종료\n";
            disconnectClient(client_fd);
            connections_.erase(client_fd);
            return;
        }

        if (payloadSize == 0) // payload 크기가 0인 경우, 패킷이 완성된 상태이므로 헤더만 버퍼에서 제거하고 다음 패킷을 처리합니다.
        {
            buf.erase(buf.begin(), buf.begin() + static_cast<long>(sizeof(int32_t))); // 헤더만 버퍼에서 제거
            continue;
        }

        size_t frameSize = sizeof(int32_t) + static_cast<size_t>(payloadSize); // 패킷의 전체 크기는 헤더(4바이트)와 payload 크기를 합한 값입니다. frameSize는 패킷의 전체 크기를 나타냅니다.
        if (buf.size() < frameSize)                                            // 버퍼에 패킷의 전체 크기보다 데이터가 적으면 패킷이 완성되지 않은 상태이므로
            return;

        std::string jsonStr(reinterpret_cast<const char *>(buf.data() + sizeof(int32_t)), // 패킷의 payload 부분을 문자열로 변환합니다.
                                                                                          // buf.data() + sizeof(int32_t)는 payload의 시작 주소를 가리키며, payloadSize는 payload의 크기를 나타냅니다.
                            static_cast<size_t>(payloadSize));

        workQueue_.push({client_fd, jsonStr});
        ; // Worker 스레드에 로직 처리 위임

        buf.erase(buf.begin(), buf.begin() + static_cast<long>(frameSize)); // 처리한 패킷만큼 버퍼에서 제거
    }
}

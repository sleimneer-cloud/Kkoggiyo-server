#include "NetClient.hpp"
#include "protocol.hpp" // PacketType 사용을 위해 추가

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static const char *HOST = "127.0.0.1";
static const uint16_t PORT = 9000;

static std::atomic<bool> g_running{true};
static std::vector<std::string> g_senders;
static std::mutex g_sendersMtx;

// 프로토콜 번호 정의 (관리자=0, 고객=1)
static const int ADMIN_TYPE = 0;

static void addSender(const std::string &senderId)
{
    if (senderId.empty())
        return;
    std::lock_guard<std::mutex> lock(g_sendersMtx);
    for (const auto &s : g_senders)
    {
        if (s == senderId)
            return;
    }
    g_senders.push_back(senderId);
}

static std::vector<std::string> getSendersSnapshot()
{
    std::lock_guard<std::mutex> lock(g_sendersMtx);
    return g_senders;
}

static void recvThread(NetClient *client)
{
    while (g_running && client->isConnected())
    {
        auto [type, j] = client->recvPacket();
        if (type < 0)
            break;

        // SC_CHAT_NOTI (201) 수신 시
        if (type == 201)
        {
            std::string from = j.value("senderId", "?");
            std::string msg = j.value("message", "");
            addSender(from);
            std::cout << "\n[상담 요청 " << from << "] " << msg << "\n> " << std::flush;
        }
    }
    g_running = false;
}

int main()
{
    std::cout << "=== 관리자 클라이언트 (Protocol: Admin=0) ===\n";

    NetClient client;
    if (!client.connect(HOST, PORT))
        return 1;

    // 관리자 ID 입력 (서버 로그의 "1234"와 맞추려면 1234 입력)
    std::cout << "관리자 ID 입력: ";
    std::string adminId;
    std::getline(std::cin, adminId);
    if (adminId.empty())
        adminId = "admin";

    // 1. 로그인 요청 (Type 0)
    nlohmann::json loginReq = {
        {"clientType", ADMIN_TYPE},
        {"userId", adminId},
        {"password", "1234"} // DB 연동 시 필요
    };

    if (!client.sendPacket(100, loginReq))
    { // CS_LOGIN_REQ
        std::cerr << "로그인 전송 실패\n";
        return 1;
    }

    auto [resType, resBody] = client.recvPacket();
    if (resBody.value("status", "") != "success")
    {
        std::cerr << "로그인 실패: " << resBody.value("message", "") << "\n";
        return 1;
    }
    std::cout << "관리자 로그인 완료.\n\n";

    std::thread recvWorker(recvThread, &client);

    while (g_running && client.isConnected())
    {
        std::cout << "1. 1대1 상담 시작 (유저 선택)\n";
        std::cout << "2. 프로그램 종료\n";
        std::cout << "선택: ";
        std::string line;
        if (!std::getline(std::cin, line))
            break;

        if (line == "2")
            break;
        if (line != "1")
            continue;

        auto senders = getSendersSnapshot();
        std::string targetId = "";

        // 대상 선택 로직
        if (senders.empty())
        {
            std::cout << "접속한 유저가 없습니다. 아이디 직접 입력: ";
            std::getline(std::cin, targetId);
        }
        else
        {
            std::cout << "\n[채팅 가능 목록]\n";
            for (size_t i = 0; i < senders.size(); ++i)
                std::cout << " " << (i + 1) << ") " << senders[i] << "\n";
            std::cout << " 0) 직접 입력\n선택: ";

            std::string sel;
            std::getline(std::cin, sel);
            int idx = std::stoi(sel == "" ? "-1" : sel);

            if (idx == 0)
            {
                std::cout << "아이디 입력: ";
                std::getline(std::cin, targetId);
            }
            else if (idx > 0 && idx <= (int)senders.size())
            {
                targetId = senders[idx - 1];
            }
        }

        if (targetId.empty())
        {
            std::cout << "대상이 지정되지 않았습니다.\n";
            continue;
        }

        std::cout << "\n--- [" << targetId << "]님과 상담 시작 (종료: 빈 줄 입력) ---\n";
        while (g_running && client.isConnected())
        {
            std::cout << "> ";
            std::string msg;
            if (!std::getline(std::cin, msg))
                break;
            if (msg.empty())
                break;

            // 🌟 수정 포인트: senderId는 관리자의 ID(123), targetId는 선택한 유저의 ID(1234)여야 합니다.
            nlohmann::json chatReq = {
                {"clientType", 0},      // 관리자(0) [cite: 17]
                {"senderId", adminId},  // 관리자 본인의 ID (예: "123") [cite: 17]
                {"targetId", targetId}, // 대화 상대방인 고객의 ID (예: "1234")
                {"message", msg}        // 전송할 메시지 [cite: 17]
            };

            // 전송 전 로그를 찍어서 확인해보세요
            std::cout << "DEBUG: 전송 패킷 -> " << chatReq.dump() << std::endl;

            if (!client.sendPacket(200, chatReq)) // CS_CHAT_REQ [cite: 17]
            {
                std::cerr << "전송 실패\n";
                break;
            }
        }
    }

    g_running = false;
    client.close();
    if (recvWorker.joinable())
        recvWorker.join();
    return 0;
}
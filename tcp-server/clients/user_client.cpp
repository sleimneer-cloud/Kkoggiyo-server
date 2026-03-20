/**
 * 사용자(고객)용 C++ 콘솔 클라이언트
 * - 메뉴: 회원가입 / 로그인
 * - 로그인 후 화면: 1대1 상담 / 프로그램 종료
 * - 1대1 상담: 관리자에게 메시지를 보내면 서버가 관리자에게 중계
 */
#include "NetClient.hpp"

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

static const char *HOST = "127.0.0.1";
static const uint16_t PORT = 9000;

// 패킷 타입 정의 (서버의 protocol.hpp와 일치해야 합니다)
static const int CS_LOGIN_REQ = 100;
static const int SC_LOGIN_RES = 101;
static const int CS_REGISTER_REQ = 102; // 회원가입 요청 패킷 번호 (임의 지정)
static const int SC_REGISTER_RES = 103; // 회원가입 응답 패킷 번호 (임의 지정)
static const int CS_CHAT_REQ = 200;
static const int SC_CHAT_NOTI = 201;

static std::atomic<bool> g_running{true};

static void recvThread(NetClient *client)
{
    while (g_running && client->isConnected())
    {
        auto [type, j] = client->recvPacket();
        if (type < 0 || j.is_null())
            break;

        if (type == SC_CHAT_NOTI)
        {
            std::string from = j.value("senderId", "?");
            std::string msg = j.value("message", "");
            std::cout << "\n[관리자 " << from << "] " << msg << "\n";
            std::cout << "> " << std::flush;
        }
        else
        {
            std::cout << "\n[서버 알림] " << j.dump() << "\n> " << std::flush;
        }
    }
    g_running = false;
}

int main()
{
    std::cout << "=== 사용자(고객) 클라이언트 ===\n";

    NetClient client;
    if (!client.connect(HOST, PORT))
    {
        std::cerr << "서버 연결 실패. 서버가 켜져 있는지 확인하세요.\n";
        return 1;
    }
    std::cout << "서버 연결됨.\n\n";

    std::string loggedInUserId = "";

    // ---------------------------------------------------------
    // 1. 회원가입 및 로그인 메뉴 루프
    // ---------------------------------------------------------
    while (loggedInUserId.empty() && client.isConnected())
    {
        std::cout << "1. 회원가입\n";
        std::cout << "2. 로그인\n";
        std::cout << "3. 종료\n";
        std::cout << "선택 (1~3): ";

        std::string choice;
        if (!std::getline(std::cin, choice))
            break;

        if (choice == "3")
        {
            std::cout << "종료합니다.\n";
            client.close();
            return 0;
        }

        if (choice == "1")
        {
            // 회원가입 진행
            std::string id, pw, name, phone;
            std::cout << "[회원가입]\n";
            std::cout << "아이디: ";
            std::getline(std::cin, id);
            std::cout << "비밀번호: ";
            std::getline(std::cin, pw);
            std::cout << "이름: ";
            std::getline(std::cin, name);
            std::cout << "전화번호: ";
            std::getline(std::cin, phone);

            nlohmann::json regReq = {
                {"clientType", 1},
                {"userId", id},
                {"password", pw}, // DB 컬럼명 및 서버 파싱 키와 일치
                {"userName", name},
                {"phone", phone}};

            if (!client.sendPacket(CS_REGISTER_REQ, regReq))
            {
                std::cerr << "회원가입 요청 전송 실패\n";
                break;
            }

            auto [resType, resBody] = client.recvPacket();
            if (resType < 0 || resBody.is_null())
            {
                std::cerr << "응답 수신 실패\n";
                break;
            }
            std::cout << "[서버 응답] " << resBody.value("message", "") << "\n\n";
        }
        else if (choice == "2")
        {
            // 로그인 진행
            std::string id, pw;
            std::cout << "[로그인]\n";
            std::cout << "아이디: ";
            std::getline(std::cin, id);
            std::cout << "비밀번호: ";
            std::getline(std::cin, pw);

            nlohmann::json loginReq = {
                {"clientType", 1},
                {"userId", id},
                {"password", pw} // userPw 였던 부분을 password로 변경
            };

            if (!client.sendPacket(CS_LOGIN_REQ, loginReq))
            {
                std::cerr << "로그인 요청 전송 실패\n";
                break;
            }

            auto [resType, resBody] = client.recvPacket();

            if (resType < 0 || resBody.is_null())
            {
                std::cerr << "응답 수신 실패\n";
                break;
            }

            if (resType == SC_LOGIN_RES && resBody.value("status", "") == "success")
            {
                std::cout << "[서버 응답] " << resBody.value("message", "") << "\n\n";
                loggedInUserId = id; // 로그인 성공 시 아이디 저장 및 루프 탈출
            }
            else
            {
                std::cout << "[서버 응답] 로그인 실패: " << resBody.value("message", "") << "\n\n";
            }
        }
        else
        {
            std::cout << "잘못된 입력입니다.\n\n";
        }
    }

    if (loggedInUserId.empty())
    {
        client.close();
        return 0;
    }

    // ---------------------------------------------------------
    // 2. 로그인 성공 후 채팅 수신 스레드 가동
    // ---------------------------------------------------------
    std::thread recvWorker(recvThread, &client);

    // ---------------------------------------------------------
    // 3. 메인 서비스 루프 (채팅)
    // ---------------------------------------------------------
    while (g_running && client.isConnected())
    {
        std::cout << "\n=== 메인 메뉴 ===\n";
        std::cout << "1. 1대1 상담 (관리자와 채팅)\n";
        std::cout << "2. 프로그램 종료\n";
        std::cout << "선택 (1 or 2): ";
        std::string line;
        if (!std::getline(std::cin, line))
            break;

        if (line == "2")
        {
            std::cout << "종료합니다.\n";
            break;
        }
        if (line != "1")
        {
            std::cout << "1 또는 2를 입력하세요.\n";
            continue;
        }

        std::cout << "\n--- 1대1 상담 (관리자에게 문의) ---\n";
        std::cout << "메시지 입력 후 Enter. 빈 줄 입력 시 상담 종료.\n\n";

        while (g_running && client.isConnected())
        {
            std::cout << "> ";
            std::string msg;
            if (!std::getline(std::cin, msg))
            {
                g_running = false;
                break;
            }
            if (msg.empty())
            {
                std::cout << "상담을 종료합니다.\n";
                break;
            }

            nlohmann::json chatReq = {
                {"clientType", 1},
                {"senderId", loggedInUserId},
                {"message", msg}};

            if (!client.sendPacket(CS_CHAT_REQ, chatReq))
            {
                std::cerr << "전송 실패\n";
                g_running = false;
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
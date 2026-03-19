/**
 * 사장님 클라이언트
 * - TCP 서버: 로그인 / 관리자 채팅
 * - HTTP 이미지 서버: 메뉴 이미지 업로드 / 다운로드
 */
#include "NetClient.hpp"
#include "HttpClient.hpp"

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

static const char *HOST = "127.0.0.1";
static const uint16_t PORT = 9000;

static const int CS_LOGIN_REQ = 100;
static const int SC_LOGIN_RES = 101;
static const int CS_CHAT_REQ = 200;
static const int SC_CHAT_NOTI = 201;

static std::atomic<bool> g_running{true};

static void recvThread(NetClient *client)
{
    while (g_running && client->isConnected())
    {
        auto [type, j] = client->recvPacket();
        if (type < 0)
            break;

        if (type == SC_CHAT_NOTI)
        {
            std::cout << "\n[관리자] " << j.value("message", "") << "\n> " << std::flush;
        }
        else
        {
            std::cout << "\n[서버] " << j.dump() << "\n> " << std::flush;
        }
    }
    g_running = false;
}

int main()
{
    std::cout << "=== 사장님 클라이언트 ===\n\n";

    // ── 1. TCP 서버 로그인 ─────────────────────────────────
    NetClient client;
    if (!client.connect(HOST, PORT))
    {
        std::cerr << "서버 연결 실패\n";
        return 1;
    }
    std::cout << "서버 연결됨\n\n";

    std::string userId, password;
    std::cout << "아이디: ";
    std::getline(std::cin, userId);
    std::cout << "비밀번호: ";
    std::getline(std::cin, password);

    nlohmann::json loginReq = {
        {"clientType", 2}, // 2 = 사장님
        {"userId", userId},
        {"password", password}};

    if (!client.sendPacket(CS_LOGIN_REQ, loginReq))
    {
        std::cerr << "로그인 전송 실패\n";
        return 1;
    }

    auto [resType, resBody] = client.recvPacket();
    if (resType != SC_LOGIN_RES || resBody.value("status", "") != "success")
    {
        std::cerr << "로그인 실패: " << resBody.value("message", "") << "\n";
        return 1;
    }
    std::cout << "로그인 성공! " << userId << "님\n\n";

    // ── 2. 수신 스레드 시작 ────────────────────────────────
    std::thread recvWorker(recvThread, &client);

    // ── 3. 메인 메뉴 루프 ─────────────────────────────────
    while (g_running && client.isConnected())
    {
        std::cout << "\n=== 메뉴 ===\n";
        std::cout << "1. 관리자 채팅\n";
        std::cout << "2. 메뉴 이미지 업로드\n";
        std::cout << "3. 메뉴 이미지 다운로드\n";
        std::cout << "4. 종료\n";
        std::cout << "선택: ";

        std::string choice;
        if (!std::getline(std::cin, choice))
            break;
        if (choice == "4")
            break;

        // ── 채팅 ──────────────────────────────────────────
        if (choice == "1")
        {
            std::cout << "\n--- 관리자 채팅 (빈 줄 입력 시 종료) ---\n";
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
                    break;

                nlohmann::json chatReq = {
                    {"clientType", 2},
                    {"senderId", userId},
                    {"message", msg}};
                if (!client.sendPacket(CS_CHAT_REQ, chatReq))
                {
                    std::cerr << "전송 실패\n";
                    g_running = false;
                    break;
                }
            }
        }

        // ── 이미지 업로드 ─────────────────────────────────
        else if (choice == "2")
        {
            std::string filePath, tmp;
            int menuId, storeId;

            std::cout << "이미지 파일 경로: ";
            std::getline(std::cin, filePath);
            std::cout << "메뉴 ID: ";
            std::getline(std::cin, tmp);
            menuId = std::stoi(tmp);
            std::cout << "가게 ID: ";
            std::getline(std::cin, tmp);
            storeId = std::stoi(tmp);

            std::string imgPath = HttpClient::uploadImage(userId, menuId, storeId, filePath);
            if (!imgPath.empty())
                std::cout << "업로드 성공! 경로: " << imgPath << "\n";
            else
                std::cout << "업로드 실패\n";
        }

        // ── 이미지 다운로드 ───────────────────────────────
        else if (choice == "3")
        {
            std::string tmp, savePath;
            int menuId;

            std::cout << "메뉴 ID: ";
            std::getline(std::cin, tmp);
            menuId = std::stoi(tmp);
            std::cout << "저장 경로 (예: /home/lms/result.jpg): ";
            std::getline(std::cin, savePath);

            std::string saved = HttpClient::downloadImage(menuId, savePath);
            if (!saved.empty())
                std::cout << "다운로드 성공! 파일: " << saved << "\n";
            else
                std::cout << "다운로드 실패\n";
        }
    }

    g_running = false;
    client.close();
    if (recvWorker.joinable())
        recvWorker.join();
    std::cout << "종료합니다.\n";
    return 0;
}
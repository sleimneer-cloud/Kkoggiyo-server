/**
 * 라이더 클라이언트
 * - TCP 서버: 로그인 / 회원가입 / 관리자 채팅
 */
#include "NetClient.hpp"
#include "protocol.hpp" // 공통 프로토콜 헤더

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

static const char *HOST = "10.10.10.113"; // 서버 IP에 맞게 수정 필요
static const uint16_t PORT = 9000;

static std::atomic<bool> g_running{true};

// ── 서버 메시지 수신 스레드 ────────────────────────────────
static void recvThread(NetClient *client)
{
    while (g_running && client->isConnected())
    {
        auto [type, j] = client->recvPacket();
        if (type < 0)
            break;

        if (type == static_cast<int>(PacketType::SC_CHAT_NOTI))
        {
            std::cout << "\n[관리자] " << j.value("message", "") << "\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_RIDER_ORDER_LIST_RES))
        {
            std::cout << "\n=== [배달 가능 주문 목록] ===\n";
            auto orders = j.value("orders", nlohmann::json::array());
            if (orders.empty()) {
                std::cout << "현재 배달 가능한 주문이 없습니다.\n";
            } else {
                for (const auto& o : orders) {
                    std::cout << "------------------------------------------\n";
                    std::cout << "주문번호: " << o.value("order_id", 0) 
                              << " | 가게: " << o.value("store_name", "") 
                              << " | 상태: " << o.value("status", "") << "\n";
                    std::cout << "주문일시: " << o.value("order_date", "") << "\n";
                }
                std::cout << "------------------------------------------\n";
            }
            std::cout << "> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_ORDER_NOTI))
        {
            std::cout << "\n🔔 [새로운 배달 알람!] \n" << j.dump(4) << "\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_ORDER_STATUS))
        {
            std::cout << "\n📢 [주문 상태 변경] " << j.value("message", "") 
                      << " (주문ID: " << j.value("order_id", 0) << ")\n> " << std::flush;
        }
        else
        {
            std::cout << "\n[서버 응답] " << j.dump() << "\n> " << std::flush;
        }
    }
    g_running = false;
}

int main()
{
    std::cout << "=== 라이더 클라이언트 ===\n\n";

    // ── 1. TCP 서버 연결 ─────────────────────────────────
    NetClient client;
    if (!client.connect(HOST, PORT))
    {
        std::cerr << "서버 연결 실패\n";
        return 1;
    }
    std::cout << "서버 연결됨\n\n";

    // ── 2. 로그인 및 회원가입 루프 ──────────────────────────
    bool isAuthenticated = false;
    std::string loggedInUserId;

    while (!isAuthenticated && g_running)
    {
        std::cout << "=== 라이더 접속 메뉴 ===\n";
        std::cout << "1. 로그인\n";
        std::cout << "2. 회원가입\n";
        std::cout << "3. 종료\n";
        std::cout << "선택: ";

        std::string authChoice;
        if (!std::getline(std::cin, authChoice))
            break;

        if (authChoice == "3")
        {
            std::cout << "프로그램을 종료합니다.\n";
            return 0;
        }
        else if (authChoice == "2") // 🚀 라이더 회원가입
        {
            std::string regId, regPw, regName, regPhone;
            std::cout << "\n[회원가입] 아이디: ";
            std::getline(std::cin, regId);
            std::cout << "[회원가입] 비밀번호: ";
            std::getline(std::cin, regPw);
            std::cout << "[회원가입] 이름: ";
            std::getline(std::cin, regName);
            std::cout << "[회원가입] 전화번호(배달 연락용): ";
            std::getline(std::cin, regPhone);

            // 라이더는 ClientType::RIDER 사용
            nlohmann::json regReq = {
                {"clientType", static_cast<int>(ClientType::RIDER)},
                {"userId", regId},
                {"password", regPw},
                {"userName", regName},
                {"phone", regPhone}};

            if (!client.sendPacket(static_cast<int>(PacketType::CS_REGISTER_REQ), regReq))
            {
                std::cerr << "회원가입 요청 전송 실패\n\n";
                continue;
            }

            auto [resType, resBody] = client.recvPacket();
            if (resType == static_cast<int>(PacketType::SC_REGISTER_RES) && resBody.value("status", "") == "success")
            {
                std::cout << "\n🎉 회원가입 성공! 이제 로그인해주세요.\n\n";
            }
            else
            {
                std::cerr << "\n❌ 회원가입 실패: " << resBody.value("message", "알 수 없는 오류") << "\n\n";
            }
        }
        else if (authChoice == "1") // 🔑 라이더 로그인
        {
            std::string inputId, inputPw;
            std::cout << "\n[로그인] 아이디: ";
            std::getline(std::cin, inputId);
            std::cout << "[로그인] 비밀번호: ";
            std::getline(std::cin, inputPw);

            nlohmann::json loginReq = {
                {"clientType", static_cast<int>(ClientType::RIDER)},
                {"userId", inputId},
                {"password", inputPw}};

            if (!client.sendPacket(static_cast<int>(PacketType::CS_LOGIN_REQ), loginReq))
            {
                std::cerr << "로그인 전송 실패\n\n";
                continue;
            }

            auto [resType, resBody] = client.recvPacket();
            if (resType == static_cast<int>(PacketType::SC_LOGIN_RES) && resBody.value("status", "") == "success")
            {
                std::cout << "\n🛵 라이더 로그인 성공! " << inputId << "님, 안전 운행하세요!\n\n";
                loggedInUserId = inputId;
                isAuthenticated = true;
            }
            else
            {
                std::cerr << "\n❌ 로그인 실패: " << resBody.value("message", "정보가 일치하지 않습니다.") << "\n\n";
            }
        }
        else
        {
            std::cout << "잘못된 입력입니다. 다시 선택해주세요.\n\n";
        }
    }

    if (!isAuthenticated)
        return 0;

    std::string userId = loggedInUserId;

    // ── 3. 수신 스레드 시작 ────────────────────────────────
    std::thread recvWorker(recvThread, &client);

    // ── 4. 메인 메뉴 루프 ─────────────────────────────────
    while (g_running && client.isConnected())
    {
        std::cout << "\n=== 라이더 메뉴 ===\n";
        std::cout << "1. 관리자 채팅 (문의하기)\n";
        std::cout << "2. 배달 가능 주문 목록 보기\n";
        std::cout << "3. 배차 신청 (주문 선택)\n";
        std::cout << "4. 종료\n";
        std::cout << "선택: ";

        std::string choice;
        if (!std::getline(std::cin, choice))
            break;
        if (choice == "4")
            break;

        try {
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
                        {"clientType", static_cast<int>(ClientType::RIDER)},
                        {"senderId", userId},
                        {"message", msg}};

                    if (!client.sendPacket(static_cast<int>(PacketType::CS_CHAT_REQ), chatReq))
                    {
                        std::cerr << "전송 실패\n";
                        g_running = false;
                        break;
                    }
                }
            }
            // ── 주문 조회 ───────────────────────────────────
            else if (choice == "2")
            {
                client.sendPacket(static_cast<int>(PacketType::CS_RIDER_ORDER_LIST_REQ), {});
                std::cout << "사용 가능한 주문 목록을 요청했습니다.\n";
            }
            // ── 배차 신청 ───────────────────────────────────
            else if (choice == "3")
            {
                std::string orderIdStr;
                std::cout << "배차받을 주문 ID: ";
                std::getline(std::cin, orderIdStr);

                nlohmann::json assignReq = {{"order_id", std::stoi(orderIdStr)}};
                client.sendPacket(static_cast<int>(PacketType::CS_ORDER_ASSIGN_REQ), assignReq);
            }
        } catch (const nlohmann::json::exception &e) {
            std::cerr << "\n[오류] 데이터 처리 중 예외 발생: " << e.what() << "\n";
            std::cin.clear();
        } catch (const std::exception &e) {
            std::cerr << "\n[오류] 입력 오류: " << e.what() << "\n";
        }
    }

    g_running = false;
    client.close();
    if (recvWorker.joinable())
        recvWorker.join();
    std::cout << "프로그램을 종료합니다. 수고하셨습니다!\n";
    return 0;
}
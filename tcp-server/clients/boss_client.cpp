/**
 * 사장님 클라이언트
 * - TCP 서버: 로그인 / 회원가입 / 관리자 채팅
 * - HTTP 이미지 서버: 메뉴 이미지 업로드 / 다운로드
 */
#include "NetClient.hpp"
#include "HttpClient.hpp"
#include "protocol.hpp" // ✅ 공통 프로토콜 헤더 사용

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

static const char *HOST = "10.10.10.113";
static const uint16_t PORT = 9000;

static std::atomic<bool> g_running{true};

static void recvThread(NetClient *client)
{
    while (g_running && client->isConnected())
    {
        auto [type, j] = client->recvPacket();
        if (type < 0)
            break;

        // ✅ 수정: PacketType enum class 형변환
        if (type == static_cast<int>(PacketType::SC_CHAT_NOTI))
        {
            std::cout << "\n[관리자] " << j.value("message", "") << "\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_OWNER_ORDER_LIST_RES))
        {
            std::cout << "\n=== [상세 주문 현황] ===\n";
            auto orders = j.value("orders", nlohmann::json::array());
            if (orders.empty()) {
                std::cout << "진행 중인 주문이 없습니다.\n";
            } else {
                for (const auto& o : orders) {
                    std::cout << "------------------------------------------\n";
                    std::cout << "주문번호: " << o.value("order_id", 0) 
                              << " | 상태: " << o.value("status", "") 
                              << " | 일시: " << o.value("order_date", "") << "\n";
                    std::cout << "고객번호: " << o.value("customer_id", 0) << "\n";
                    std::cout << "요청사항: " << o.value("request_msg", "") << "\n";
                    std::cout << "[주문 항목]\n";
                    auto items = o.value("items", nlohmann::json::array());
                    for (const auto& item : items) {
                        std::cout << " - " << item.value("menu_name", "알 수 없음") 
                                  << " (" << item.value("quantity", 0) << "개) : " 
                                  << item.value("price", 0) << "원\n";
                    }
                }
                std::cout << "------------------------------------------\n";
            }
            std::cout << "> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_ORDER_NOTI))
        {
            std::cout << "\n🔔 [새로운 주문 알림!] \n" << j.dump(4) << "\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_ORDER_STATUS))
        {
            std::cout << "\n📢 [주문 상태 변경] " << j.value("message", "") 
                      << " (주문ID: " << j.value("order_id", 0) << ")\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_STORE_REGISTER_RES))
        {
            std::cout << "\n🏪 [가게 등록 결과] " << j.value("message", "") << "\n> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_OWNER_STORE_LIST_RES))
        {
            std::cout << "\n=== [내 가게 목록] ===\n";
            auto stores = j.value("stores", nlohmann::json::array());
            if (stores.empty()) {
                std::cout << "등록된 가게가 없습니다.\n";
            } else {
                for (const auto& s : stores) {
                    std::cout << "ID: " << s.value("store_id", 0) 
                              << " | 이름: " << s.value("store_name", "") << "\n";
                }
            }
            std::cout << "> " << std::flush;
        }
        else if (type == static_cast<int>(PacketType::SC_MENU_REGISTER_RES))
        {
            std::cout << "\n📜 [메뉴 등록 결과] " << j.value("message", "") << "\n> " << std::flush;
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
    std::cout << "=== 사장님 클라이언트 ===\n\n";

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
        std::cout << "=== 접속 메뉴 ===\n";
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
        else if (authChoice == "2") // 🚀 회원가입
        {
            std::string regId, regPw, regName, regPhone;
            std::cout << "\n[회원가입] 아이디: ";
            std::getline(std::cin, regId);
            std::cout << "[회원가입] 비밀번호: ";
            std::getline(std::cin, regPw);
            std::cout << "[회원가입] 이름: ";
            std::getline(std::cin, regName);
            std::cout << "[회원가입] 전화번호: ";
            std::getline(std::cin, regPhone);
            std::string regStoreName, regStoreAddr, regLicense;
            std::cout << "[회원가입] 가게 이름: ";
            std::getline(std::cin, regStoreName);
            std::cout << "[회원가입] 가게 주소: ";
            std::getline(std::cin, regStoreAddr);
            std::cout << "[회원가입] 사업자 등록 번호: ";
            std::getline(std::cin, regLicense);

            // ✅ 수정: ClientType::OWNER 적용
            nlohmann::json regReq = {
                {"clientType", static_cast<int>(ClientType::OWNER)},
                {"userId", regId},
                {"password", regPw}, // 클라이언트단 해싱이 필요하다면 여기서 적용
                {"userName", regName},
                {"phone", regPhone},
                {"storeName", regStoreName},
                {"storeAddress", regStoreAddr},
                {"license", regLicense}};

            // ✅ 수정: PacketType::CS_REGISTER_REQ 적용
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
        else if (authChoice == "1") // 🔑 로그인
        {
            std::string inputId, inputPw;
            std::cout << "\n[로그인] 아이디: ";
            std::getline(std::cin, inputId);
            std::cout << "[로그인] 비밀번호: ";
            std::getline(std::cin, inputPw);

            nlohmann::json loginReq = {
                {"clientType", static_cast<int>(ClientType::OWNER)},
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
                std::cout << "\n✅ 로그인 성공! " << inputId << "님 환영합니다.\n\n";
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
        std::cout << "\n=== 메뉴 ===\n";
        std::cout << "1. 관리자 채팅\n";
        std::cout << "2. 메뉴 이미지 업로드\n";
        std::cout << "3. 메뉴 이미지 다운로드\n";
        std::cout << "4. 가게 상태 변경 (오픈/마감)\n";
        std::cout << "5. 주문 관리\n";
        std::cout << "6. 가게 등록\n";
        std::cout << "7. 내 가게 목록 및 ID 조회\n";
        std::cout << "8. 메뉴 등록\n";
        std::cout << "9. 종료\n";
        std::cout << "선택: ";

        std::string choice;
        if (!std::getline(std::cin, choice))
            break;
        if (choice == "9")
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
                        {"clientType", static_cast<int>(ClientType::OWNER)},
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
            // ── 이미지 업로드 ─────────────────────────────────
            else if (choice == "2")
            {
                std::string filePath, tmp, ownerIdStr;
                int menuId, storeId;

                std::cout << "이미지 파일 경로: ";
                std::getline(std::cin, filePath);
                std::cout << "메뉴 ID: ";
                std::getline(std::cin, tmp);
                menuId = std::stoi(tmp);
                std::cout << "가게 ID: ";
                std::getline(std::cin, tmp);
                storeId = std::stoi(tmp);

                std::cout << "사장님 고유번호(owner_id, 예: 15): ";
                std::getline(std::cin, ownerIdStr);

                std::string imgPath = HttpClient::uploadImage(ownerIdStr, menuId, storeId, filePath);
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
            // ── 가게 상태 변경 ───────────────────────────────
            else if (choice == "4")
            {
                std::string sStoreId;
                std::cout << "\n[가게 상태 변경]\n가게 ID: ";
                std::getline(std::cin, sStoreId);

                std::cout << "1. 오픈 하기\n2. 마감 하기\n선택: ";
                std::string sChoice;
                std::getline(std::cin, sChoice);
                int isOpen = (sChoice == "1") ? 1 : 0;

                nlohmann::json openReq = {
                    {"isOpen", isOpen},
                    {"storeId", std::stoi(sStoreId)}
                };
                if (client.sendPacket(static_cast<int>(PacketType::CS_STORE_OPEN_REQ), openReq)) {
                    std::cout << "요청 전송 완료. 서버 응답 대기 중...\n";
                }
            }
            // ── 주문 관리 ───────────────────────────────────
            else if (choice == "5")
            {
                std::cout << "\n--- 주문 관리 메뉴 ---\n";
                std::cout << "1. 현재 주문 현황 보기 (전체 내역)\n";
                std::cout << "2. 주문 수락 (조리 시작)\n";
                std::cout << "3. 조리 완료 (픽업 요청)\n";
                std::cout << "선택: ";
                std::string oChoice;
                std::getline(std::cin, oChoice);

                if (oChoice == "1") {
                    client.sendPacket(static_cast<int>(PacketType::CS_OWNER_ORDER_LIST_REQ), {});
                    std::cout << "주문 목록을 요청했습니다.\n";
                }
                else if (oChoice == "2") {
                    std::string orderIdStr, estTimeStr;
                    std::cout << "수락할 주문 ID: ";
                    std::getline(std::cin, orderIdStr);
                    std::cout << "예상 조리 시간(분): ";
                    std::getline(std::cin, estTimeStr);

                    nlohmann::json acceptReq = {
                        {"order_id", std::stoi(orderIdStr)},
                        {"estimated_time", std::stoi(estTimeStr)},
                        {"accepted", true}
                    };
                    client.sendPacket(static_cast<int>(PacketType::CS_ORDER_ACCEPT), acceptReq);
                }
                else if (oChoice == "3") {
                    std::string orderIdStr;
                    std::cout << "완료 처리할 주문 ID: ";
                    std::getline(std::cin, orderIdStr);

                    nlohmann::json readyReq = {{"order_id", std::stoi(orderIdStr)}};
                    client.sendPacket(static_cast<int>(PacketType::CS_ORDER_READY), readyReq);
                }
            }
            // ── 가게 등록 ───────────────────────────────────
            else if (choice == "6")
            {
                std::string sName, sAddr, sRegNum, sCatId;
                std::cout << "\n[가게 등록]\n";
                std::cout << "가게 이름: ";
                std::getline(std::cin, sName);
                std::cout << "가게 주소: ";
                std::getline(std::cin, sAddr);
                std::cout << "사업자 등록 번호: ";
                std::getline(std::cin, sRegNum);
                std::cout << "업종 카테고리 ID (1:한식, 2:카페, 3:중식 등): ";
                std::getline(std::cin, sCatId);

                nlohmann::json regReq = {
                    {"name", sName},
                    {"address", sAddr},
                    {"regNum", sRegNum},
                    {"categoryId", std::stoi(sCatId)}
                };

                if (client.sendPacket(static_cast<int>(PacketType::CS_STORE_REGISTER_REQ), regReq)) {
                    std::cout << "가게 등록 요청을 보냈습니다. 서버 응답을 기다려주세요.\n";
                }
            }
            // ── 내 가게 목록 조회 ──────────────────────────────
            else if (choice == "7")
            {
                client.sendPacket(static_cast<int>(PacketType::CS_OWNER_STORE_LIST_REQ), {});
                std::cout << "내 가게 목록을 요청했습니다.\n";
            }
            // ── 메뉴 등록 ───────────────────────────────────
            else if (choice == "8")
            {
                std::string sStoreId, sMenuName, sPrice, sImgPath;
                std::cout << "\n[메뉴 등록 (가게 ID를 먼저 확인하세요)]\n";
                std::cout << "가게 ID: ";
                std::getline(std::cin, sStoreId);
                std::cout << "메뉴 이름: ";
                std::getline(std::cin, sMenuName);
                std::cout << "가격: ";
                std::getline(std::cin, sPrice);
                std::cout << "이미지 경로 (생략 가능): ";
                std::getline(std::cin, sImgPath);

                nlohmann::json menuReq = {
                    {"store_id", std::stoi(sStoreId)},
                    {"menu_name", sMenuName},
                    {"price", std::stoi(sPrice)},
                    {"img_path", sImgPath}
                };

                if (client.sendPacket(static_cast<int>(PacketType::CS_MENU_REGISTER_REQ), menuReq)) {
                    std::cout << "메뉴 등록 요청을 보냈습니다.\n";
                }
            }
        } catch (const nlohmann::json::exception &e) {
            std::cerr << "\n[오류] 입력값에 처리할 수 없는 문자(깨진 한글 등)가 포함되어 있습니다. 다시 시도해주세요.\n\n";
            std::cout << "상세 오류: " << e.what() << "\n";
            std::cin.clear();
        } catch (const std::exception &e) {
            std::cerr << "\n[오류] 입력 중 오류 발생: " << e.what() << "\n";
        }
    }

    g_running = false;
    client.close();
    if (recvWorker.joinable())
        recvWorker.join();
    std::cout << "종료합니다.\n";
    return 0;
}
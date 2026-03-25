#include "handlers/AuthHandler.hpp"

#include <string>
#include <iostream>

#include "SessionManager.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"

#include "services/CustomerAuthService.hpp"
#include "services/BossAuthService.hpp"
#include "services/RiderAuthService.hpp" // ✅ 주석 해제

// ---------------------------------------------------------
// 로그인
// ---------------------------------------------------------
void AuthHandler::handleLogin(int client_fd, const nlohmann::json &j) const
{
    std::string id = j.value("userId", "");
    int cType = j.value("clientType", -1);

    bool loginSuccess = false;

    switch (static_cast<ClientType>(cType))
    {
    case ClientType::CUSTOMER:
    {
        CustomerAuthService customerSvc;
        loginSuccess = customerSvc.processLogin(client_fd, j);
        break;
    }
    case ClientType::OWNER:
    {
        BossAuthService bossSvc;
        loginSuccess = bossSvc.processLogin(client_fd, j);
        break;
    }
    case ClientType::RIDER: // ✅ 주석 해제 — RiderAuthService 위임
    {
        RiderAuthService riderSvc;
        loginSuccess = riderSvc.processLogin(client_fd, j);
        break;
    }
    case ClientType::ADMIN:
    {
        // TODO: 관리자 DB 검증 추가 필요 (현재 무조건 성공)
        std::cout << "[AuthHandler] 관리자(" << id << ") 로그인 요청 접수\n";
        loginSuccess = true;
        break;
    }
    default:
        std::cerr << "[AuthHandler] 알 수 없는 clientType: " << cType << "\n";
        loginSuccess = false;
        break;
    }

    if (loginSuccess)
    {
        SessionManager::getInstance().addSession(client_fd, cType, id);
        nlohmann::json res = {{"status", "success"}, {"message", "로그인 환영합니다!"}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_LOGIN_RES, res);
    }
    else
    {
        std::cerr << "[AuthHandler] 로그인 실패 (FD: " << client_fd << ")\n";
        nlohmann::json res = {{"status", "fail"}, {"message", "로그인 정보가 올바르지 않거나 누락되었습니다."}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_LOGIN_RES, res);
    }
}

// ---------------------------------------------------------
// 회원가입
// ---------------------------------------------------------
void AuthHandler::handleRegister(int client_fd, const nlohmann::json &j) const
{
    int cType = j.value("clientType", -1);
    bool registerSuccess = false;

    switch (static_cast<ClientType>(cType))
    {
    case ClientType::CUSTOMER:
    {
        CustomerAuthService customerSvc;
        registerSuccess = customerSvc.processRegister(j);
        break;
    }
    case ClientType::OWNER: // ✅ 사장님(OWNER) 회원가입 라우팅 추가!
    {
        BossAuthService bossSvc;
        registerSuccess = bossSvc.processRegister(j);
        break;
    }
    case ClientType::RIDER: // ✅ 라이더 회원가입 추가
    {
        RiderAuthService riderSvc;
        registerSuccess = riderSvc.processRegister(j);
        break;
    }
    default:
        // OWNER 가입은 BossAuthService::processRegister가 아직 미구현(return false)
        // ADMIN 가입은 서버 직접 DB 삽입으로 처리 (앱 가입 불가)
        std::cerr << "[AuthHandler] 미구현 또는 허용되지 않는 가입 요청. clientType: " << cType << "\n";
        break;
    }

    if (registerSuccess)
    {
        nlohmann::json res = {{"status", "success"}, {"message", "회원가입이 완료되었습니다."}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_REGISTER_RES, res);
    }
    else
    {
        nlohmann::json res = {{"status", "fail"}, {"message", "회원가입 실패 (중복된 아이디 또는 필수 정보 누락)"}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_REGISTER_RES, res);
    }
}

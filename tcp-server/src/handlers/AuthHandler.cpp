#include "handlers/AuthHandler.hpp"

#include <string>
#include <iostream> // 로그 출력을 위해 추가

#include "SessionManager.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"

// 🌟 새로 만들 서비스 클래스들의 헤더를 포함합니다.
#include "services/CustomerAuthService.hpp"
#include "services/BossAuthService.hpp"
// #include "services/RiderAuthService.hpp" // 아직 미구현

// 로그인 처리
void AuthHandler::handleLogin(int client_fd, const nlohmann::json &j) const
{
    std::string id = j.value("userId", "");
    // 기본값을 0에서 -1로 변경하여, 타입이 안 들어왔을 때 기본 통과되는 것을 방지합니다.
    int cType = j.value("clientType", -1);

    bool loginSuccess = false;

    // 1. clientType에 따라 알맞은 서비스로 위임 (분기 처리)
    switch (static_cast<ClientType>(cType))
    {
    case ClientType::CUSTOMER:
    { // 고객
        CustomerAuthService customerSvc;
        loginSuccess = customerSvc.processLogin(client_fd, j);
        break;
    }
    case ClientType::OWNER:
    { // 사장님
        BossAuthService bossSvc;
        loginSuccess = bossSvc.processLogin(client_fd, j);
        break;
    }
    // case 3:
    // { // 라이더
    //     RiderAuthService riderSvc;
    //     loginSuccess = riderSvc.processLogin(client_fd, j);
    //     break;
    // }
    case ClientType::ADMIN:
    {
        // 관리자는 별도 검증 로직이 당장 필요 없다면 기본 성공 처리
        std::cout << "[AuthHandler] 관리자(" << id << ") 로그인 요청 접수\n";
        loginSuccess = true;
        break;
    }
    default:
        std::cerr << "[AuthHandler] 알 수 없는 clientType 입니다: " << cType << "\n";
        loginSuccess = false;
        break;
    }

    // 2. 공통 후처리: 서비스 계층에서 로그인이 성공했다고 판정했을 때만 세션 등록
    if (loginSuccess)
    {
        SessionManager::getInstance().addSession(client_fd, cType, id);

        nlohmann::json res = {{"status", "success"}, {"message", "로그인 환영합니다!"}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_LOGIN_RES, res);
    }
    // 3. 로그인 실패 시 처리
    else
    {
        std::cerr << "[AuthHandler] 로그인 실패 (FD: " << client_fd << ")\n";

        nlohmann::json res = {{"status", "fail"}, {"message", "로그인 정보가 올바르지 않거나 누락되었습니다."}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_LOGIN_RES, res);
    }
}

// 회원가입
void AuthHandler::handleRegister(int client_fd, const nlohmann::json &j) const
{
    int cType = j.value("clientType", -1);
    bool registerSuccess = false;

    // 현재 handleLogin의 번호 규칙(고객=1)에 맞춤
    if (cType == static_cast<int>(ClientType::CUSTOMER))
    {
        CustomerAuthService customerSvc;
        registerSuccess = customerSvc.processRegister(j);
    }
    else
    {
        std::cerr << "[AuthHandler] 알 수 없는 clientType 이거나 아직 미구현된 가입 요청입니다.\n";
    }

    if (registerSuccess)
    {
        nlohmann::json res = {{"status", "success"}, {"message", "회원가입이 완료되었습니다."}};
        // SC_REGISTER_RES (응답 패킷 번호) 전송
        PacketHandler::sendPacket(client_fd, PacketType::SC_REGISTER_RES, res);
    }
    else
    {
        nlohmann::json res = {{"status", "fail"}, {"message", "회원가입 실패 (중복된 아이디 또는 필수 정보 누락)"}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_REGISTER_RES, res);
    }
}
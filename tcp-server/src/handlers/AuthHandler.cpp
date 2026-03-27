#include "handlers/AuthHandler.hpp"

#include <string>
#include <iostream>

#include "SessionManager.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"

#include "services/CustomerAuthService.hpp"
#include "services/BossAuthService.hpp"
#include "services/RiderAuthService.hpp"
#include "services/AdminAuthService.hpp"

// ---------------------------------------------------------
// 로그인
// ---------------------------------------------------------
void AuthHandler::handleLogin(int client_fd, const nlohmann::json &j) const
{
    std::string id = j.value("userId", "");
    int cType = j.value("clientType", -1);

    bool loginSuccess = false;
    std::string outName;
    std::string outMsg;

    switch (static_cast<ClientType>(cType))
    {
    case ClientType::CUSTOMER:
    {
        CustomerAuthService customerSvc;                              
        loginSuccess = customerSvc.processLogin(client_fd, j, outName, outMsg);
        
        if (loginSuccess)
        {
            SessionManager::getInstance().addSession(client_fd, cType, id);
            nlohmann::json res = {
                {"status",  "success"},
                {"message", "로그인 환영합니다!"},
                {"userId",  id},
                {"name",    outName}
            };
            PacketHandler::sendPacket(client_fd, PacketType::USER_LOGIN_RES, res);
        }
        else
        {
            nlohmann::json res = {
                {"status",  "fail"},
                {"message", outMsg.empty() ? "로그인 정보가 올바르지 않거나 누락되었습니다." : outMsg}
            };
            PacketHandler::sendPacket(client_fd, PacketType::USER_LOGIN_RES, res);
        }
        return;
    }
    case ClientType::OWNER:
    {
        BossAuthService bossSvc;
        loginSuccess = bossSvc.processLogin(client_fd, j, outMsg);
        break;
    }
    case ClientType::RIDER:
    {
        RiderAuthService riderSvc;
        loginSuccess = riderSvc.processLogin(client_fd, j, outMsg);
        break;
    }
    case ClientType::ADMIN:
    {
        AdminAuthService adminSvc;
        loginSuccess = adminSvc.processLogin(client_fd, j, outMsg);
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
        nlohmann::json res = {{"status", "success"}, {"message", "로그인 환영합니다!"}, {"userId", id}, {"name", outName}};
        PacketHandler::sendPacket(client_fd, PacketType::SC_LOGIN_RES, res);
    }
    else
    {
        std::cerr << "[AuthHandler] 로그인 실패 (FD: " << client_fd << ")\n";
        nlohmann::json res = {{"status", "fail"}, {"message", outMsg.empty() ? "로그인 정보가 올바르지 않거나 누락되었습니다." : outMsg}};
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
        bool success = customerSvc.processRegister(j);

        nlohmann::json res;
        res["status"]  = success ? "success" : "fail";
        res["message"] = success ? "인증번호를 발송했습니다."
                                : "회원가입 요청 실패 (필드 누락 또는 이메일 오류)";
        
        PacketHandler::sendPacket(client_fd, PacketType::USER_REGISTER_RES, res);
            return;
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

// 인증번호 확인
void AuthHandler::handleVerify(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool success = customerSvc.processVerify(client_fd, j);

    nlohmann::json res;
    res["status"]  = success ? "success" : "fail";
    res["message"] = success ? "회원가입이 완료되었습니다."
                             : "인증번호가 일치하지 않거나 만료되었습니다.";
    
    PacketHandler::sendPacket(client_fd, PacketType::USER_VERIFY_RES, res);                         
}

// 닉네임 변경
void AuthHandler::handleNameChange(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool success = customerSvc.processNameChange(j);

    nlohmann::json res;
    res["status"]  = success ? "success" : "fail";
    res["message"] = success ? "닉네임이 변경되었습니다." : "닉네임 변경 실패";
    PacketHandler::sendPacket(client_fd,
        PacketType::USER_NAME_ALT_RES, res);
}

// 탈퇴 시 비밀번호 확인
void AuthHandler::handleWithdrawVerify(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool success = customerSvc.processWithdrawVerify(j);

    nlohmann::json res;
    res["status"]  = success ? "success" : "fail";
    res["message"] = success ? "인증번호를 발송했습니다."
                             : "비밀번호가 일치하지 않습니다.";
    PacketHandler::sendPacket(client_fd,
        PacketType::USER_WITHDRAW_PW_RES, res);
}

//  회원탈퇴
void AuthHandler::handleWithdraw(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool success = customerSvc.processWithdraw(j);

    nlohmann::json res;
    res["status"]  = success ? "success" : "fail";
    res["message"] = success ? "회원탈퇴가 완료되었습니다."
                             : "회원탈퇴 처리 중 오류가 발생했습니다.";
    PacketHandler::sendPacket(client_fd, PacketType::USER_WITHDRAW_RES, res);
}

// 아이디 중복 확인
void AuthHandler::handleIdCheck(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool available = customerSvc.processIdCheck(j);

    nlohmann::json res;
    res["status"]  = available ? "success" : "fail";
    res["message"] = available ? "사용 가능한 아이디입니다."
                               : "이미 사용중인 아이디입니다.";
    PacketHandler::sendPacket(client_fd, PacketType::USER_ID_CHECK_RES, res);
}

// 주소 추가
void AuthHandler::handleAddAddress(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    bool success = customerSvc.processAddAddress(j);

    nlohmann::json res;
    res["status"]  = success ? "success" : "fail";
    res["message"] = success ? "주소가 추가되었습니다."
                             : "주소 추가에 실패했습니다.";
    PacketHandler::sendPacket(client_fd, PacketType::USER_ADDR_ADD_RES, res);
}

// 주소 목록 조회
void AuthHandler::handleGetAddresses(int client_fd, const nlohmann::json &j) const
{
    CustomerAuthService customerSvc;
    auto addresses = customerSvc.processGetAddresses(j);

    nlohmann::json res;
    res["status"]    = "success";
    res["addresses"] = addresses;
    PacketHandler::sendPacket(client_fd, PacketType::USER_ADDR_LIST_RES, res);
}
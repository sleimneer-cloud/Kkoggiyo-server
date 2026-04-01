#pragma once
#include "json.hpp"
#include "services/MenuService.hpp"

using json = nlohmann::json;

class MenuHandler
{
public:
    MenuHandler() = default;
    ~MenuHandler() = default;

    // CS_MENU_REGISTER_REQ (4950) 패킷 처리
    void handleMenuRegister(int clientFd, const json &j);

private:
    MenuService menuSvc_;
};

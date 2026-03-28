#include "handlers/MenuHandler.hpp"
#include "PacketHandler.hpp"
#include "protocol.hpp"
#include "SessionManager.hpp"

void MenuHandler::handleMenuRegister(int clientFd, const json &j)
{
    // { "store_id": 1, "menu_name": "...", "price": 1000, "img_path": "..." }
    
    int storeId = j.value("store_id", 0);
    std::string name = j.value("menu_name", "");
    int price = j.value("price", 0);
    std::string img = j.value("img_path", "");

    if (storeId == 0 || name.empty() || price <= 0) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_MENU_REGISTER_RES,
                                  {{"status", "fail"}, {"message", "필수 정보가 누락되었습니다. (가게ID, 이름, 가격)"}});
        return;
    }

    std::string outMsg;
    if (menuSvc_.registerMenu(storeId, name, price, img, outMsg)) {
        PacketHandler::sendPacket(clientFd, PacketType::SC_MENU_REGISTER_RES,
                                  {{"status", "success"}, {"message", outMsg}});
    } else {
        PacketHandler::sendPacket(clientFd, PacketType::SC_MENU_REGISTER_RES,
                                  {{"status", "fail"}, {"message", outMsg}});
    }
}

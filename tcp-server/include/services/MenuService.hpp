#pragma once
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class MenuService
{
public:
    MenuService() = default;
    ~MenuService() = default;

    // 메뉴 등록
    bool registerMenu(int storeId, const std::string& menuName, int price, const std::string& imgPath, std::string& outMsg);
};

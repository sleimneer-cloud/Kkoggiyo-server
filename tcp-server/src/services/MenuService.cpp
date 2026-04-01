#include "services/MenuService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <vector>

bool MenuService::registerMenu(int storeId, const std::string& menuName, int price, const std::string& imgPath, std::string& outMsg)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // menu 테이블에 데이터 삽입
    std::string qMenu = "INSERT INTO menu (store_id, menu_name, price, img_path, is_soldout) VALUES (?, ?, ?, ?, 0)";
    std::vector<std::string> pMenu = {
        std::to_string(storeId),
        menuName,
        std::to_string(price),
        imgPath
    };

    if (DBHelper::executeUpdate(conn, qMenu, pMenu)) {
        outMsg = "메뉴가 성공적으로 등록되었습니다.";
        std::cout << "[MenuService] 가게ID(" << storeId << ") 새 메뉴 등록: " << menuName << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return true;
    } else {
        outMsg = "메뉴 등록 실패 (DB 오류)";
        std::cerr << "[MenuService] 메뉴 등록 실패. store_id=" << storeId << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }
}

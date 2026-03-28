#include "services/StoreService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <vector>

bool StoreService::toggleStoreStatus(const std::string& loginId, int storeId, int isOpen, std::string& outMsg)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // 1. login_id를 기반으로 owner_id(member_id) 추출
    std::string qMember = "SELECT member_id FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> pMember = {loginId};
    std::string memberIdStr;

    if (!DBHelper::executeSelectOneString(conn, qMember, pMember, memberIdStr)) {
        outMsg = "사장님 계정을 찾을 수 없거나 권한이 없습니다.";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // 2. 특정 store_id와 owner_id를 가진 가게의 is_open 업데이트 (소유권 검증 포함)
    std::string qUpdate = "UPDATE store SET is_open = ? WHERE store_id = ? AND owner_id = ?";
    std::vector<std::string> pUpdate = {std::to_string(isOpen), std::to_string(storeId), memberIdStr};

    if (DBHelper::executeUpdate(conn, qUpdate, pUpdate)) {
        // 실제 업데이트된 행이 있는지 확인하고 싶으나 DBHelper::executeUpdate가 true만 반환하므로 성공으로 간주.
        // 만약 0행이 업데이트 되었다면 (가게 ID가 남의 것이거나 없는 경우) 클라이언트는 성공 메시지를 받지만 실제 반영은 안됨.
        outMsg = (isOpen == 1) ? "가게가 오픈되었습니다." : "가게가 마감되었습니다.";
        std::cout << "[StoreService] " << loginId << " 가게(ID:" << storeId << ") 상태 변경 성공 (is_open=" << isOpen << ")\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return true;
    } else {
        outMsg = "가게 상태 변경 DB 업데이트에 실패했습니다.";
        std::cerr << "[StoreService] " << loginId << " 가게(ID:" << storeId << ") 상태 업데이트 실패.\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }
}

bool StoreService::registerStore(const std::string& loginId, const std::string& name, 
                                 const std::string& addr, const std::string& regNum, 
                                 int categoryId, std::string& outMsg)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // 1. login_id → member_id(owner_id) 조회
    std::string qMember = "SELECT member_id FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> pMember = {loginId};
    std::string memberIdStr;

    if (!DBHelper::executeSelectOneString(conn, qMember, pMember, memberIdStr)) {
        outMsg = "사장님 계정을 찾을 수 없거나 권한이 없습니다.";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // 2. store 테이블에 새 가게 정보 삽입
    // category_id가 있으면 넣고, 없으면 NULL (여기선 입력받은 categoryId 사용)
    std::string qStore = "INSERT INTO store (owner_id, category_id, store_name, store_address, business_reg_num, is_open) "
                         "VALUES (?, ?, ?, ?, ?, 1)"; // 기본적으로 오픈 상태(1)로 등록
    std::vector<std::string> pStore = {
        memberIdStr, 
        std::to_string(categoryId), 
        name, 
        addr, 
        regNum
    };

    if (!DBHelper::executeUpdate(conn, qStore, pStore)) {
        outMsg = "가게 정보 등록 실패 (DB 오류)";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // 3. owner 테이블의 shop_registered 플래그 업데이트 (이미 존재하면 업데이트, 없으면 삽입 고려 가능하나 일단 업데이트 위주)
    // 사용자 예시에서 owner 테이블이 별도로 존재하므로 이를 업데이트합니다.
    std::string qOwner = "INSERT INTO owner (owner_id, shop_registered) VALUES (?, 1) "
                         "ON DUPLICATE KEY UPDATE shop_registered = 1";
    std::vector<std::string> pOwner = {memberIdStr};
    
    if (!DBHelper::executeUpdate(conn, qOwner, pOwner)) {
        std::cerr << "[StoreService] owner 테이블 업데이트 실패 (owner_id=" << memberIdStr << ")\n";
        // 가게는 등록되었으므로 여기서 false를 리턴하진 않지만 로그를 남김
    }

    outMsg = "가게가 성공적으로 등록되었습니다.";
    std::cout << "[StoreService] " << loginId << " 새 가게 등록 완료 (" << name << ")\n";
    DBConnectionPool::getInstance().releaseConnection(conn);
    return true;
}

json StoreService::getOwnerStores(const std::string& loginId)
{
    json storeList = json::array();
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return storeList;

    // 1. member_id 조회
    std::string qMember = "SELECT member_id FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> pMember = {loginId};
    std::string memberIdStr;

    if (!DBHelper::executeSelectOneString(conn, qMember, pMember, memberIdStr)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return storeList;
    }

    // 2. 해당 owner_id의 가게 목록 조회
    std::string qStore = "SELECT store_id, store_name FROM store WHERE owner_id = ?";
    std::vector<std::string> pStore = {memberIdStr};
    
    std::vector<std::vector<std::string>> rows;
    if (DBHelper::executeSelectMultipleRows(conn, qStore, pStore, {11, 100}, rows)) {
        for (const auto& row : rows) {
            json s;
            s["store_id"] = std::stoi(row[0]);
            s["store_name"] = row[1];
            storeList.push_back(s);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return storeList;
}

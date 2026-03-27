#include "services/StoreService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>
#include <vector>

bool StoreService::toggleStoreStatus(const std::string& loginId, int isOpen, std::string& outMsg)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // 1. login_id를 기반으로 owner_id(현재는 member_id와 1:1 대응) 추출
    std::string qMember = "SELECT member_id FROM member WHERE login_id = ? AND role = 2";
    std::vector<std::string> pMember = {loginId};
    std::string memberIdStr;

    if (!DBHelper::executeSelectOneString(conn, qMember, pMember, memberIdStr)) {
        outMsg = "사장님 계정을 찾을 수 없거나 권한이 없습니다.";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }

    // 2. store 테이블에서 해당 owner_id를 가진 가게의 is_open 업데이트
    std::string qUpdate = "UPDATE store SET is_open = ? WHERE owner_id = ?";
    std::vector<std::string> pUpdate = {std::to_string(isOpen), memberIdStr};

    if (DBHelper::executeUpdate(conn, qUpdate, pUpdate)) {
        outMsg = (isOpen == 1) ? "가게가 오픈되었습니다." : "가게가 마감되었습니다.";
        std::cout << "[StoreService] " << loginId << " 가게 상태 변경 성공 (is_open=" << isOpen << ")\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return true;
    } else {
        outMsg = "가게 상태 변경 DB 업데이트에 실패했습니다. (가게 정보가 등록되어 있는지 확인하세요)";
        std::cerr << "[StoreService] " << loginId << " 상태 업데이트 실패.\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return false;
    }
}

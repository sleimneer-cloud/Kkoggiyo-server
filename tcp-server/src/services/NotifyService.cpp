#include "services/NotifyService.hpp"
#include "DBConnectionPool.h"
#include <iostream>
#include <mysql/mysql.h>

// store_id로 사장님 login_id 조회 후 fd 반환 헬퍼
static int getStoreFd(int storeId)
{
    MYSQL* conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return -1;

    std::string q = "SELECT m.login_id FROM store s "
                    "JOIN member m ON s.owner_id = m.member_id "
                    "WHERE s.store_id = " + std::to_string(storeId);

    if (mysql_query(conn, q.c_str()))
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return -1;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);
    std::string loginId = row ? row[0] : "";
    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (loginId.empty()) return -1;
    return SessionManager::getInstance().getFdByUserId(loginId);
}

// member_id로 login_id 조회 후 fd 반환 헬퍼
static int getMemberFd(int memberId)
{
    MYSQL* conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return -1;

    std::string q = "SELECT login_id FROM member WHERE member_id = "
                    + std::to_string(memberId);

    if (mysql_query(conn, q.c_str()))
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return -1;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);
    std::string loginId = row ? row[0] : "";
    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (loginId.empty()) return -1;
    return SessionManager::getInstance().getFdByUserId(loginId);
}

void NotifyService::notifyStore(int storeId, int orderId, const json& orderInfo)
{
    int fd = getStoreFd(storeId);
    if (fd < 0)
    {
        std::cerr << "[NotifyService] 가게 fd 없음. store_id: " << storeId << "\n";
        return;
    }

    json payload    = orderInfo;
    payload["order_id"] = orderId;
    payload["message"]  = "신규 주문이 들어왔습니다.";

    PacketHandler::sendPacket(fd, PacketType::SC_ORDER_NOTI, payload);
    std::cout << "[NotifyService] 가게(" << storeId << ")에 주문 알림 전송 완료\n";
}

void NotifyService::notifyCustomer(int customerId, int orderId,
                                    const std::string& status)
{
    int fd = getMemberFd(customerId);
    if (fd < 0)
    {
        std::cerr << "[NotifyService] 고객 fd 없음. customer_id: " << customerId << "\n";
        return;
    }

    json payload = {
        {"order_id", orderId},
        {"status",   status},
        {"message",  "주문 상태가 변경되었습니다: " + status}
    };

    PacketHandler::sendPacket(fd, PacketType::SC_ORDER_STATUS, payload);
    std::cout << "[NotifyService] 고객(" << customerId << ")에 상태 알림 전송 완료\n";
}

void NotifyService::notifyRider(int riderId, int orderId, const json& orderInfo)
{
    // riders.rider_id = member.member_id 구조 가정
    int fd = getMemberFd(riderId);
    if (fd < 0)
    {
        std::cerr << "[NotifyService] 라이더 fd 없음. rider_id: " << riderId << "\n";
        return;
    }

    json payload        = orderInfo;
    payload["order_id"] = orderId;
    payload["message"]  = "새 배달 요청입니다.";

    PacketHandler::sendPacket(fd, PacketType::SC_ORDER_NOTI, payload);
    std::cout << "[NotifyService] 라이더(" << riderId << ")에 배달 요청 전송 완료\n";
}

void NotifyService::broadcastToRiders(int orderId, const json& orderInfo)
{
    std::vector<int> riderFds = SessionManager::getInstance().getRiderFds();
    if (riderFds.empty())
    {
        std::cout << "[NotifyService] 현재 접속 중인 라이더가 없습니다.\n";
        return;
    }

    json payload = orderInfo;
    payload["order_id"] = orderId;
    payload["message"]  = "조리가 완료되었습니다. 픽업을 요청합니다.";

    // 접속 중인 모든 라이더에게 알림 전송 (콜)
    for (int fd : riderFds)
    {
        PacketHandler::sendPacket(fd, PacketType::SC_ORDER_NOTI, payload);
    }
    std::cout << "[NotifyService] 접속 중인 라이더 " << riderFds.size() << "명에게 픽업 요청 발송 완료.\n";
}

#include <unistd.h>

void NotifyService::notifyBan(int memberId, const std::string& reason)
{
    int fd = getMemberFd(memberId);
    if (fd < 0) {
        std::cerr << "[NotifyService] 유저 fd 없음. O/L 중이 아님. member_id: " << memberId << "\n";
        return;
    }

    json payload = {
        {"status", "fail"},
        {"message", "관리자에 의해 강제 로그아웃 되었습니다. 사유: " + reason}
    };
    // 차단당했음을 로그인 실패/또는 공지용 응답으로 전송
    PacketHandler::sendPacket(fd, PacketType::SC_LOGIN_RES, payload);
    std::cout << "[NotifyService] 유저(" << memberId << ")에 차단 알림 전송 및 접속 강제 해제\n";

    SessionManager::getInstance().removeSession(fd);
    close(fd);
}

void NotifyService::notifyDispatchComplete(int orderId)
{
    // 1. 주문 정보 조회 (가게 ID, 고객 ID 필요)
    MYSQL* conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return;

    std::string q = "SELECT customer_id, store_id FROM orders WHERE order_id = " + std::to_string(orderId);
    if (mysql_query(conn, q.c_str())) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW  row = mysql_fetch_row(res);
    int customerId = row ? std::stoi(row[0]) : 0;
    int storeId    = row ? std::stoi(row[1]) : 0;
    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    if (customerId == 0 || storeId == 0) return;

    json payload = {
        {"order_id", orderId},
        {"status", "DISPATCHED"},
        {"message", "라이더 배차가 완료되었습니다. 곧 픽업이 시작됩니다."}
    };

    // 2. 사장님에게 알림
    int storeFd = getStoreFd(storeId);
    if (storeFd >= 0) {
        PacketHandler::sendPacket(storeFd, PacketType::SC_ORDER_STATUS, payload);
    }

    // 3. 고객에게 알림
    int customerFd = getMemberFd(customerId);
    if (customerFd >= 0) {
        PacketHandler::sendPacket(customerFd, PacketType::SC_ORDER_STATUS, payload);
    }

    std::cout << "[NotifyService] 주문(" << orderId << ") 배차 완료 알림(사장/고객) 전송 완료\n";
}

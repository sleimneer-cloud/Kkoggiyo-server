#include "services/AdminService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <mysql/mysql.h>
#include <iostream>
json AdminService::getUserById(int targetId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return {};

    std::string q = "SELECT member_id, login_id, name, role, email, phone, is_banned, account_num "
                    "FROM member WHERE member_id = " +
                    std::to_string(targetId);

    std::cout << "[DEBUG] 실행 쿼리: " << q << "\n";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[AdminService] getUserById 쿼리 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return {};
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    std::cout << "[DEBUG] row 결과: " << (row ? "있음" : "없음") << "\n";

    json result = {};

    if (row)
    {
        result = {
            {"RES_member_id", row[0] ? std::stoi(row[0]) : 0},
            {"RES_login_id", row[1] ? row[1] : ""},
            {"RES_name", row[2] ? row[2] : ""},
            {"RES_role", row[3] ? std::stoi(row[3]) : 0}, // [수정] 정수형 변환
            {"RES_email", row[4] ? row[4] : ""},
            {"RES_phone", row[5] ? row[5] : ""},
            {"RES_is_banned", row[6] ? std::stoi(row[6]) : 0},
            {"RES_account_num", row[7] ? row[7] : ""}};
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    std::cout << "[1004 전송 JSON] " << result.dump() << "\n";

    return result;
}

// ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
// [수정] 신규 추가: login_id 문자열로 유저 조회
// getUserById 와 동일한 구조, WHERE 조건만 다름
// ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑
json AdminService::getUserByLoginId(const std::string &loginId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return {};

    // [수정] SQL 인젝션 방지를 위해 mysql_real_escape_string 사용
    char escaped[201] = {};
    mysql_real_escape_string(conn, escaped, loginId.c_str(), loginId.size());

    std::string q = "SELECT member_id, login_id, name, role, email, phone, is_banned, account_num "
                    "FROM member WHERE login_id = '" +
                    std::string(escaped) + "'";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[AdminService] getUserByLoginId 쿼리 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return {};
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    json result = {};

    if (row)
    {
        result = {
            {"RES_member_id", row[0] ? std::stoi(row[0]) : 0},
            {"RES_login_id", row[1] ? row[1] : ""},
            {"RES_name", row[2] ? row[2] : ""},
            {"RES_role", row[3] ? std::stoi(row[3]) : 0},
            {"RES_email", row[4] ? row[4] : ""},
            {"RES_phone", row[5] ? row[5] : ""},
            {"RES_is_banned", row[6] ? std::stoi(row[6]) : 0},
            {"RES_account_num", row[7] ? row[7] : ""}};
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    std::cout << "[1004 전송 JSON] " << result.dump() << "\n";

    return result;
}

json AdminService::getOrderHistoryByTargetId(int targetId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return json::array();

    // [수정 포인트] login_id 버전과 동일하게 JOIN을 사용하여 플랫(Flat)한 구조로 데이터 한 번에 조회
    std::string q =
        "SELECT "
        "o.order_id, o.store_id, o.rider_id, o.status, o.estimated_time, "
        "o.order_date, o.request_msg, s.store_name, mn.menu_name, "
        "oi.quantity, (oi.price * oi.quantity) AS total_price "
        "FROM orders o "
        "JOIN member mb ON o.customer_id = mb.member_id "
        "JOIN store s ON o.store_id = s.store_id "
        "JOIN order_item oi ON o.order_id = oi.order_id "
        "JOIN menu mn ON oi.menu_id = mn.menu_id "
        "WHERE mb.member_id = " +
        std::to_string(targetId) + " " // <-- 숫자 ID로 검색!
                                   "ORDER BY o.order_date DESC";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[AdminService] getOrderHistoryByTargetId 쿼리 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    json ordersArr = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        json itemObj = {
            {"order_id", row[0] ? std::stoi(row[0]) : 0},
            {"store_id", row[1] ? std::stoi(row[1]) : 0},
            {"rider_id", row[2] ? std::stoi(row[2]) : 0},
            {"status", row[3] ? row[3] : ""},
            {"estimated_time", row[4] ? std::stoi(row[4]) : 0},
            {"order_date", row[5] ? row[5] : ""},
            {"request_msg", row[6] ? row[6] : ""},
            {"store_name", row[7] ? row[7] : ""},
            {"menu_name", row[8] ? row[8] : ""},
            {"quantity", row[9] ? std::stoi(row[9]) : 0},
            {"total_price", row[10] ? std::stoi(row[10]) : 0}};

        ordersArr.push_back(itemObj);
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return ordersArr;
}

json AdminService::getOrderHistoryByLoginId(const std::string &loginId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return json::array();

    // SQL 인젝션 방지
    char escaped[201] = {};
    mysql_real_escape_string(conn, escaped, loginId.c_str(), loginId.size());

    std::string q =
        "SELECT "
        "o.order_id, o.store_id, o.rider_id, o.status, o.estimated_time, "
        "o.order_date, o.request_msg, s.store_name, mn.menu_name, "
        "oi.quantity, (oi.price * oi.quantity) AS total_price "
        "FROM orders o "
        "JOIN member mb ON o.customer_id = mb.member_id "
        "JOIN store s ON o.store_id = s.store_id "
        "JOIN order_item oi ON o.order_id = oi.order_id "
        "JOIN menu mn ON oi.menu_id = mn.menu_id "
        "WHERE mb.login_id = '" +
        std::string(escaped) + "' "
                               "ORDER BY o.order_date DESC";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[AdminService] getOrderHistoryByLoginId 쿼리 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    json ordersArr = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        json itemObj = {
            {"order_id", row[0] ? std::stoi(row[0]) : 0},
            {"store_id", row[1] ? std::stoi(row[1]) : 0},
            {"rider_id", row[2] ? std::stoi(row[2]) : 0},
            {"status", row[3] ? row[3] : ""},
            {"estimated_time", row[4] ? std::stoi(row[4]) : 0},
            {"order_date", row[5] ? row[5] : ""},
            {"request_msg", row[6] ? row[6] : ""},
            {"store_name", row[7] ? row[7] : ""},
            {"menu_name", row[8] ? row[8] : ""},
            {"quantity", row[9] ? std::stoi(row[9]) : 0},
            {"total_price", row[10] ? std::stoi(row[10]) : 0}};

        ordersArr.push_back(itemObj);
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    return ordersArr;
}
int AdminService::getAdminIdByEmail(const std::string& email)
{
    MYSQL* conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return -1;

    std::string query = "SELECT admin_id FROM admin_log WHERE email = ?";
    std::vector<std::string> params = {email};
    
    std::vector<std::vector<std::string>> results;
    int adminId = -1;
    if (DBHelper::executeSelectMultipleRows(conn, query, params, {20}, results)) {
        if (!results.empty() && !results[0][0].empty()) {
            adminId = std::stoi(results[0][0]);
        }
    }
    
    DBConnectionPool::getInstance().releaseConnection(conn);
    return adminId;
}

bool AdminService::processRefund(int orderId, const std::string& amount, int adminId, std::string& outMsg, int& outCustomerId, int& outStoreId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // 1. 주문 정보 확인 (상태, 고객, 상점)
    std::string qOrder = "SELECT status, customer_id, store_id FROM orders WHERE order_id = ?";
    std::vector<std::string> pOrder = {std::to_string(orderId)};
    std::vector<std::vector<std::string>> resOrder;
    if (!DBHelper::executeSelectMultipleRows(conn, qOrder, pOrder, {20, 20, 20}, resOrder) || resOrder.empty()) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "주문을 찾을 수 없습니다.";
        return false;
    }

    std::string currentStatus = resOrder[0][0];
    if (currentStatus == "CANCELLED") {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "이미 취소된 주문입니다.";
        return false;
    }

    outCustomerId = std::stoi(resOrder[0][1]);
    outStoreId = std::stoi(resOrder[0][2]);

    // 2. 취소 처리
    std::string qUpdate = "UPDATE orders SET status = 'CANCELLED' WHERE order_id = ?";
    std::vector<std::string> pUpdate = {std::to_string(orderId)};
    if (!DBHelper::executeUpdate(conn, qUpdate, pUpdate)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "주문 상태 업데이트 실패";
        return false;
    }

    // 3. chat_queue 조회 (가장 최근 해당 주문의 큐)
    std::string qQueue = "SELECT queue_id FROM chat_queue WHERE order_id = ? ORDER BY queue_id DESC LIMIT 1";
    std::vector<std::string> pQueue = {std::to_string(orderId)};
    std::vector<std::vector<std::string>> resQueue;
    int queueId = 0; // fallback 임시 queue
    if (DBHelper::executeSelectMultipleRows(conn, qQueue, pQueue, {20}, resQueue) && !resQueue.empty() && !resQueue[0][0].empty()) {
        queueId = std::stoi(resQueue[0][0]);
    } else {
        // queue_id 가 없으면 임의로 최신 큐나 하나 만들어야 할 수 있지만, 여기선 없으면 임시 큐를 만든다고 가정
        std::string qInsertQ = "INSERT INTO chat_queue (order_id, member_id, issue_type, status, assigned_admin_id) VALUES (?, ?, 'ETC', 'RESOLVED', ?)";
        std::vector<std::string> pInsertQ = {std::to_string(orderId), std::to_string(outCustomerId), std::to_string(adminId)};
        if (DBHelper::executeUpdate(conn, qInsertQ, pInsertQ)) {
           queueId = mysql_insert_id(conn); 
        }
    }

    // 4. admin_action 기록
    std::string qAction = "INSERT INTO admin_action (queue_id, admin_id, action_type, target_type, target_id, memo) VALUES (?, ?, 'REFUND', 'ORDER', ?, '관리자 환불 처리')";
    std::vector<std::string> pAction = {std::to_string(queueId), std::to_string(adminId), std::to_string(orderId)};
    DBHelper::executeUpdate(conn, qAction, pAction);

    DBConnectionPool::getInstance().releaseConnection(conn);
    outMsg = "환불 및 취소 처리가 완료되었습니다.";
    return true;
}

bool AdminService::processBanUser(int memberId, const std::string& reason, int adminId, std::string& outMsg)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) {
        outMsg = "DB 연결 실패";
        return false;
    }

    // 1. 유저 존재 확인
    std::string qUser = "SELECT is_banned FROM member WHERE member_id = ?";
    std::vector<std::string> pUser = {std::to_string(memberId)};
    std::vector<std::vector<std::string>> resUser;
    if (!DBHelper::executeSelectMultipleRows(conn, qUser, pUser, {10}, resUser) || resUser.empty()) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "유저를 찾을 수 없습니다.";
        return false;
    }

    int isBanned = std::stoi(resUser[0][0]);
    if (isBanned == 1) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "이미 차단된 유저입니다.";
        return false;
    }

    // 2. 차단 상태 업데이트
    std::string qUpdate = "UPDATE member SET is_banned = 1 WHERE member_id = ?";
    std::vector<std::string> pUpdate = {std::to_string(memberId)};
    if (!DBHelper::executeUpdate(conn, qUpdate, pUpdate)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        outMsg = "유저 상태 업데이트 실패";
        return false;
    }

    // 3. chat_queue 조회 (유저 ID 기준)
    std::string qQueue = "SELECT queue_id FROM chat_queue WHERE member_id = ? ORDER BY queue_id DESC LIMIT 1";
    std::vector<std::string> pQueue = {std::to_string(memberId)};
    std::vector<std::vector<std::string>> resQueue;
    int queueId = 0;
    if (DBHelper::executeSelectMultipleRows(conn, qQueue, pQueue, {20}, resQueue) && !resQueue.empty() && !resQueue[0][0].empty()) {
        queueId = std::stoi(resQueue[0][0]);
    } else {
        // 유저의 가장 최근 order_id를 큐에 사용하기 위해 조회
        std::string qOrder = "SELECT order_id FROM orders WHERE customer_id = ? ORDER BY order_id DESC LIMIT 1";
        std::vector<std::string> pOrder = {std::to_string(memberId)};
        std::vector<std::vector<std::string>> resOrder;
        std::string targetOrderId = "1"; // 최후의 보루 (아예 주문이 없는 신규가입 유저일 경우 1번 주문 사용)
        
        if (DBHelper::executeSelectMultipleRows(conn, qOrder, pOrder, {20}, resOrder) && !resOrder.empty() && !resOrder[0][0].empty()) {
            targetOrderId = resOrder[0][0];
        }

        std::string qInsertQ = "INSERT INTO chat_queue (order_id, member_id, issue_type, status, assigned_admin_id) VALUES (?, ?, 'ETC', 'RESOLVED', ?)";
        std::vector<std::string> pInsertQ = {targetOrderId, std::to_string(memberId), std::to_string(adminId)};
        if (DBHelper::executeUpdate(conn, qInsertQ, pInsertQ)) {
           queueId = mysql_insert_id(conn); 
        } else {
            // 실패 시 최후의 방법: 그냥 존재하는 아무 queue_id나 가져다 기록
            std::string qFallback = "SELECT queue_id FROM chat_queue ORDER BY queue_id DESC LIMIT 1";
            std::vector<std::vector<std::string>> resFallback;
            if (DBHelper::executeSelectMultipleRows(conn, qFallback, {}, {20}, resFallback) && !resFallback.empty() && !resFallback[0][0].empty()) {
                queueId = std::stoi(resFallback[0][0]);
            }
        }
    }

    // 4. admin_action 기록
    std::string qAction = "INSERT INTO admin_action (queue_id, admin_id, action_type, target_type, target_id, memo) VALUES (?, ?, 'BAN_MEMBER', 'MEMBER', ?, ?)";
    std::vector<std::string> pAction = {std::to_string(queueId), std::to_string(adminId), std::to_string(memberId), reason};
    DBHelper::executeUpdate(conn, qAction, pAction);

    DBConnectionPool::getInstance().releaseConnection(conn);
    outMsg = "유저 차단 처리가 완료되었습니다.";
    return true;
}

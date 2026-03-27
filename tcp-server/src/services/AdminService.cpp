#include "services/AdminService.hpp"
#include "DBConnectionPool.h"
#include <mysql/mysql.h>
#include <iostream>

// 기존: member_id 로 조회 (변경 없음)
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
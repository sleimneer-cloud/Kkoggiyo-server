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
    if (!conn) return json::array();

    std::string q = "SELECT order_id, store_id, rider_id, status, estimated_time, request_msg, order_date "
                    "FROM orders WHERE customer_id = " + std::to_string(targetId) + " ORDER BY order_date DESC";

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
        int orderId = row[0] ? std::stoi(row[0]) : 0;
        json orderObj = {
            {"order_id", orderId},
            {"store_id", row[1] ? std::stoi(row[1]) : 0},
            {"rider_id", row[2] ? std::stoi(row[2]) : 0},
            {"status", row[3] ? row[3] : ""},
            {"estimated_time", row[4] ? std::stoi(row[4]) : 0},
            {"request_msg", row[5] ? row[5] : ""},
            {"order_date", row[6] ? row[6] : ""},
            {"items", json::array()}
        };
        ordersArr.push_back(orderObj);
    }
    mysql_free_result(res);

    for (auto& orderObj : ordersArr)
    {
        int orderId = orderObj["order_id"];
        std::string iq = "SELECT order_item_id, menu_id, quantity, price FROM order_item WHERE order_id = " + std::to_string(orderId);
        if (mysql_query(conn, iq.c_str()) == 0)
        {
            MYSQL_RES *ires = mysql_store_result(conn);
            MYSQL_ROW irow;
            json itemsArr = json::array();
            while ((irow = mysql_fetch_row(ires)))
            {
                itemsArr.push_back({
                    {"order_item_id", irow[0] ? std::stoi(irow[0]) : 0},
                    {"menu_id", irow[1] ? std::stoi(irow[1]) : 0},
                    {"quantity", irow[2] ? std::stoi(irow[2]) : 0},
                    {"price", irow[3] ? std::stoi(irow[3]) : 0}
                });
            }
            orderObj["items"] = itemsArr;
            mysql_free_result(ires);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return ordersArr;
}

json AdminService::getOrderHistoryByLoginId(const std::string &loginId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return json::array();

    char escaped[201] = {};
    mysql_real_escape_string(conn, escaped, loginId.c_str(), loginId.size());

    std::string q = "SELECT o.order_id, o.store_id, o.rider_id, o.status, o.estimated_time, o.request_msg, o.order_date "
                    "FROM orders o JOIN member m ON o.customer_id = m.member_id "
                    "WHERE m.login_id = '" + std::string(escaped) + "' ORDER BY o.order_date DESC";

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
        int orderId = row[0] ? std::stoi(row[0]) : 0;
        json orderObj = {
            {"order_id", orderId},
            {"store_id", row[1] ? std::stoi(row[1]) : 0},
            {"rider_id", row[2] ? std::stoi(row[2]) : 0},
            {"status", row[3] ? row[3] : ""},
            {"estimated_time", row[4] ? std::stoi(row[4]) : 0},
            {"request_msg", row[5] ? row[5] : ""},
            {"order_date", row[6] ? row[6] : ""},
            {"items", json::array()}
        };
        ordersArr.push_back(orderObj);
    }
    mysql_free_result(res);

    for (auto& orderObj : ordersArr)
    {
        int orderId = orderObj["order_id"];
        std::string iq = "SELECT order_item_id, menu_id, quantity, price FROM order_item WHERE order_id = " + std::to_string(orderId);
        if (mysql_query(conn, iq.c_str()) == 0)
        {
            MYSQL_RES *ires = mysql_store_result(conn);
            MYSQL_ROW irow;
            json itemsArr = json::array();
            while ((irow = mysql_fetch_row(ires)))
            {
                itemsArr.push_back({
                    {"order_item_id", irow[0] ? std::stoi(irow[0]) : 0},
                    {"menu_id", irow[1] ? std::stoi(irow[1]) : 0},
                    {"quantity", irow[2] ? std::stoi(irow[2]) : 0},
                    {"price", irow[3] ? std::stoi(irow[3]) : 0}
                });
            }
            orderObj["items"] = itemsArr;
            mysql_free_result(ires);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return ordersArr;
}

#include "services/OrderService.hpp"
#include "DBConnectionPool.h"
#include <iostream>
#include <mysql/mysql.h>

int OrderService::createOrder(int customerId, int storeId, const std::string &requestMsg, const json &items)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return -1;

    // 1. 트랜잭션 시작
    mysql_query(conn, "START TRANSACTION");

    std::string q = "INSERT INTO orders (customer_id, store_id, request_msg, status) "
                    "VALUES (" +
                    std::to_string(customerId) + ", " + std::to_string(storeId) + ", '" + requestMsg + "', 'PENDING')";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[OrderService] orders INSERT 실패: " << mysql_error(conn) << "\n";
        mysql_query(conn, "ROLLBACK"); // 롤백
        DBConnectionPool::getInstance().releaseConnection(conn);
        return -1;
    }

    int orderId = static_cast<int>(mysql_insert_id(conn));

    for (const auto &item : items)
    {
        int menuId = item.value("menu_id", 0);
        int quantity = item.value("quantity", 1);
        int price = item.value("price", 0);

        std::string iq = "INSERT INTO order_item (order_id, menu_id, quantity, price) "
                         "VALUES (" +
                         std::to_string(orderId) + ", " + std::to_string(menuId) + ", " + std::to_string(quantity) + ", " + std::to_string(price) + ")";

        if (mysql_query(conn, iq.c_str()))
        {
            std::cerr << "[OrderService] order_item INSERT 실패: " << mysql_error(conn) << "\n";
            mysql_query(conn, "ROLLBACK"); // 롤백
            DBConnectionPool::getInstance().releaseConnection(conn);
            return -1;
        }
    }

    // 2. 모든 쿼리 성공 시 커밋
    mysql_query(conn, "COMMIT");
    DBConnectionPool::getInstance().releaseConnection(conn);
    std::cout << "[OrderService] 주문 생성 완료. order_id: " << orderId << "\n";
    return orderId;
}

bool OrderService::updateStatus(int orderId, const std::string &status)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string q = "UPDATE orders SET status = '" + status +
                    "' WHERE order_id = " + std::to_string(orderId);

    bool ok = (mysql_query(conn, q.c_str()) == 0);
    if (!ok)
        std::cerr << "[OrderService] updateStatus 실패: " << mysql_error(conn) << "\n";

    DBConnectionPool::getInstance().releaseConnection(conn);
    return ok;
}

json OrderService::getOrder(int orderId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return {};

    std::string q = "SELECT o.order_id, o.customer_id, o.store_id, o.rider_id, "
                    "o.status, o.estimated_time, o.request_msg, o.order_date "
                    "FROM orders o WHERE o.order_id = " +
                    std::to_string(orderId);

    if (mysql_query(conn, q.c_str()))
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return {};
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    json result = {};

    if (row)
    {
        result = {
            {"order_id", row[0] ? std::stoi(row[0]) : 0},
            {"customer_id", row[1] ? std::stoi(row[1]) : 0},
            {"store_id", row[2] ? std::stoi(row[2]) : 0},
            {"rider_id", row[3] ? std::stoi(row[3]) : 0},
            {"status", row[4] ? row[4] : ""},
            {"estimated_time", row[5] ? std::stoi(row[5]) : 0},
            {"request_msg", row[6] ? row[6] : ""},
            {"order_date", row[7] ? row[7] : ""}};
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

json OrderService::getOrdersByCustomer(int customerId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return json::array();

    std::string q = "SELECT order_id, store_id, status, order_date "
                    "FROM orders WHERE customer_id = " +
                    std::to_string(customerId) +
                    " ORDER BY order_date DESC";

    if (mysql_query(conn, q.c_str()))
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    json result = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        result.push_back({{"order_id", row[0] ? std::stoi(row[0]) : 0},
                          {"store_id", row[1] ? std::stoi(row[1]) : 0},
                          {"status", row[2] ? row[2] : ""},
                          {"order_date", row[3] ? row[3] : ""}});
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

json OrderService::getOrdersByStore(int storeId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return json::array();

    std::string q = "SELECT order_id, customer_id, status, estimated_time, order_date "
                    "FROM orders WHERE store_id = " +
                    std::to_string(storeId) +
                    " AND status NOT IN ('COMPLETED','CANCELLED')"
                    " ORDER BY order_date ASC";

    if (mysql_query(conn, q.c_str()))
    {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    json result = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res)))
    {
        result.push_back({{"order_id", row[0] ? std::stoi(row[0]) : 0},
                          {"customer_id", row[1] ? std::stoi(row[1]) : 0},
                          {"status", row[2] ? row[2] : ""},
                          {"estimated_time", row[3] ? std::stoi(row[3]) : 0},
                          {"order_date", row[4] ? row[4] : ""}});
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

bool OrderService::assignRider(int orderId, int riderId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string q = "UPDATE orders SET rider_id = " + std::to_string(riderId) +
                    ", status = 'DELIVERING'"
                    " WHERE order_id = " +
                    std::to_string(orderId);

    bool ok = (mysql_query(conn, q.c_str()) == 0);
    if (!ok)
        std::cerr << "[OrderService] assignRider 실패: " << mysql_error(conn) << "\n";

    DBConnectionPool::getInstance().releaseConnection(conn);
    return ok;
}

bool OrderService::unassignRider(int orderId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return false;

    std::string q = "UPDATE orders SET rider_id = NULL "
                    "WHERE order_id = " +
                    std::to_string(orderId);

    bool ok = (mysql_query(conn, q.c_str()) == 0);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return ok;
}

std::vector<int> OrderService::getStoreIdsByOwnerLoginId(const std::string& loginId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    std::vector<int> storeIds;
    if (!conn) return storeIds;

    // 1. login_id를 통해 숫자 member_id 조회
    std::string qMember = "SELECT member_id FROM member WHERE login_id = '" + loginId + "'";
    if (mysql_query(conn, qMember.c_str())) {
        std::cerr << "[OrderService] member_id 조회 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return storeIds;
    }
    MYSQL_RES *resM = mysql_store_result(conn);
    MYSQL_ROW rowM = mysql_fetch_row(resM);
    int memberId = (rowM && rowM[0]) ? std::stoi(rowM[0]) : 0;
    mysql_free_result(resM);

    if (memberId == 0) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return storeIds;
    }

    // 2. 조회된 member_id를 owner_id로 사용하여 모든 store_id 조회
    std::string qStore = "SELECT store_id FROM store WHERE owner_id = " + std::to_string(memberId);
    if (mysql_query(conn, qStore.c_str())) {
        std::cerr << "[OrderService] store_id 목록 조회 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return storeIds;
    }

    MYSQL_RES *resS = mysql_store_result(conn);
    MYSQL_ROW rowS;
    while ((rowS = mysql_fetch_row(resS))) {
        if (rowS[0]) {
            storeIds.push_back(std::stoi(rowS[0]));
        }
    }
    mysql_free_result(resS);

    DBConnectionPool::getInstance().releaseConnection(conn);
    return storeIds;
}

int OrderService::getStoreIdByOwnerLoginId(const std::string& loginId)
{
    std::vector<int> ids = getStoreIdsByOwnerLoginId(loginId);
    if (ids.empty()) return 0;
    return ids[0]; // 첫 번째 가게 ID 반환
}

json OrderService::getOrdersWithItemsByStore(int storeId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return json::array();

    // 1. 주문 목록 조회
    std::string q = "SELECT order_id, customer_id, status, estimated_time, order_date, request_msg "
                    "FROM orders WHERE store_id = " + std::to_string(storeId) +
                    " AND status NOT IN ('COMPLETED','CANCELLED') "
                    "ORDER BY order_date ASC";

    if (mysql_query(conn, q.c_str())) {
        std::cerr << "[OrderService] 주문 목록 조회 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    json result = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        int orderId = std::stoi(row[0]);
        json orderObj = {
            {"order_id", orderId},
            {"customer_id", row[1] ? std::stoi(row[1]) : 0},
            {"status", row[2] ? row[2] : ""},
            {"estimated_time", row[3] ? std::stoi(row[3]) : 0},
            {"order_date", row[4] ? row[4] : ""},
            {"request_msg", row[5] ? row[5] : ""},
            {"items", json::array()}
        };
        result.push_back(orderObj);
    }
    mysql_free_result(res);

    // 2. 각 주문의 상세 아이템들 채우기 (동일 커넥션 재사용)
    for (auto& orderObj : result) {
        int orderId = orderObj["order_id"];
        std::string iq = "SELECT oi.menu_id, m.menu_name, oi.quantity, oi.price "
                         "FROM order_item oi "
                         "JOIN menu m ON oi.menu_id = m.menu_id "
                         "WHERE oi.order_id = " + std::to_string(orderId);

        if (mysql_query(conn, iq.c_str()) == 0) {
            MYSQL_RES *resItem = mysql_store_result(conn);
            if (resItem) {
                MYSQL_ROW rowItem;
                while ((rowItem = mysql_fetch_row(resItem))) {
                    orderObj["items"].push_back({
                        {"menu_id", std::stoi(rowItem[0])},
                        {"menu_name", rowItem[1] ? rowItem[1] : ""},
                        {"quantity", std::stoi(rowItem[2])},
                        {"price", std::stoi(rowItem[3])}
                    });
                }
                mysql_free_result(resItem);
            }
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

json OrderService::getAvailableOrdersForRider()
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return json::array();

    // 조리 중 혹은 조리 완료된 주문 중 라이더가 아직 안 붙은 것들
    std::string q = "SELECT o.order_id, s.store_name as store_name, o.status, o.order_date "
                    "FROM orders o "
                    "JOIN store s ON o.store_id = s.store_id "
                    "WHERE o.status IN ('COOKING', 'READY') AND o.rider_id IS NULL "
                    "ORDER BY o.order_date ASC";

    if (mysql_query(conn, q.c_str())) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return json::array();
    }

    MYSQL_RES *res = mysql_store_result(conn);
    json result = json::array();
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res))) {
        result.push_back({
            {"order_id", std::stoi(row[0])},
            {"store_name", row[1] ? row[1] : ""},
            {"status", row[2] ? row[2] : ""},
            {"order_date", row[3] ? row[3] : ""}
        });
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

bool OrderService::assignRiderToOrder(int orderId, int riderId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return false;

    // 배차 확정: 라이더 ID 업데이트
    std::string q = "UPDATE orders SET rider_id = " + std::to_string(riderId) +
                    " WHERE order_id = " + std::to_string(orderId);

    bool ok = (mysql_query(conn, q.c_str()) == 0);
    DBConnectionPool::getInstance().releaseConnection(conn);
    return ok;
}
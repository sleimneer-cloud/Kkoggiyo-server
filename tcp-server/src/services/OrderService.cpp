#include "services/OrderService.hpp"
#include "DBConnectionPool.h"
#include <iostream>
#include <mysql/mysql.h>

int OrderService::createOrder(int customerId, int storeId,
                              const std::string &requestMsg,
                              const json &items)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return -1;

    // 1. orders INSERT
    std::string q = "INSERT INTO orders (customer_id, store_id, request_msg, status) "
                    "VALUES (" +
                    std::to_string(customerId) + ", " + std::to_string(storeId) + ", '" + requestMsg + "', 'PENDING')";

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[OrderService] createOrder 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return -1;
    }

    int orderId = static_cast<int>(mysql_insert_id(conn));

    // 2. order_item INSERT (items 배열 순회)
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
            std::cerr << "[OrderService] order_item INSERT 실패: "
                      << mysql_error(conn) << "\n";
            // 주문 자체를 롤백
            std::string del = "DELETE FROM orders WHERE order_id = " + std::to_string(orderId);
            mysql_query(conn, del.c_str());
            DBConnectionPool::getInstance().releaseConnection(conn);
            return -1;
        }
    }

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
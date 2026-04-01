#include "services/ClientService.hpp"
#include "DBConnectionPool.h"
#include "DBHelper.hpp"
#include <iostream>

// ─────────────────────────────────────────
//  가게 목록 조회
// ─────────────────────────────────────────
std::vector<nlohmann::json> ClientService::processGetStores(
    const nlohmann::json &payload)
{
    std::vector<nlohmann::json> result;
    if (!payload.contains("category") || !payload.contains("district"))
        return result;

    std::string category = payload["category"];
    std::string district = payload["district"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return result;

    std::string query;
    std::vector<std::string> params;

    if (category == "전체") {
        query =
            "SELECT s.store_id, s.store_name, s.store_address, "
            "       c.category_name, s.is_open "
            "FROM store s "
            "JOIN category c ON s.category_id = c.category_id "
            "WHERE s.store_address LIKE ? ";
        params = {"%" + district + "%"};
    } else {
        query =
            "SELECT s.store_id, s.store_name, s.store_address, "
            "       c.category_name, s.is_open "
            "FROM store s "
            "JOIN category c ON s.category_id = c.category_id "
            "WHERE c.category_name = ? "
            "AND s.store_address LIKE ? ";
        params = {category, "%" + district + "%"};
    }

    std::vector<int> colSizes = {10, 100, 300, 50, 5};
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(conn, query, params, colSizes, rows)) {
        for (const auto &row : rows) {
            nlohmann::json item;
            item["storeId"]      = std::stoi(row[0]);
            item["storeName"]    = row[1];
            item["storeAddress"] = row[2];
            item["category"]     = row[3];
            item["isOpen"]       = (row[4] == "1");
            result.push_back(item);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    std::cout << "[ClientService] 가게 조회: "
              << category << " / " << district
              << " / " << result.size() << "건\n";
    return result;
}

// ─────────────────────────────────────────
//  메뉴 목록 조회
// ─────────────────────────────────────────
std::vector<nlohmann::json> ClientService::processGetMenus(
    const nlohmann::json &payload)
{
    std::vector<nlohmann::json> result;
    if (!payload.contains("storeId")) return result;

    std::string storeId;
    if (payload["storeId"].is_number()) {
        storeId = std::to_string(payload["storeId"].get<int>());
    } else {
        storeId = payload["storeId"].get<std::string>();
    }

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return result;

    std::string query =
        "SELECT menu_id, menu_name, price, img_path, is_soldout "
        "FROM menu WHERE store_id = ?";
    std::vector<std::string> params = {storeId};
    std::vector<int> colSizes = {10, 100, 10, 500, 5};
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(conn, query, params, colSizes, rows)) {
        for (const auto &row : rows) {
            nlohmann::json item;
            item["menuId"]     = std::stoi(row[0]);
            item["menuName"]   = row[1];
            item["price"]      = std::stoi(row[2]);
            item["imgPath"]    = row[3];
            item["isSoldout"]  = (row[4] == "1");
            result.push_back(item);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

// ─────────────────────────────────────────
//  주문 전송
// ─────────────────────────────────────────
nlohmann::json ClientService::processPlaceOrder(
    const nlohmann::json &payload)
{
    nlohmann::json result;
    result["success"] = false;

    if (!payload.contains("storeId") ||
        !payload.contains("items")   ||
        !payload.contains("totalPrice")) {
        return result;
    }

    std::string storeId       = std::to_string(payload["storeId"].get<int>());
    std::string totalPrice    = std::to_string(payload["totalPrice"].get<int>());
    std::string deliveryFee   = std::to_string(
        payload.value("deliveryFee", 0));
    std::string roadAddr      = payload.value("roadAddress",   "");
    std::string detailAddr    = payload.value("detailAddress", "");
    std::string payMethod     = payload.value("paymentMethod", "카드");
    std::string storeReq      = payload.value("storeRequest",  "");
    std::string riderReq      = payload.value("riderRequest",  "");

    // 회원/비회원 처리
    std::string memberId = "NULL";
    if (payload.contains("userId") &&
        !std::string(payload["userId"]).empty())
    {
        std::string userId = payload["userId"];
        MYSQL *conn = DBConnectionPool::getInstance().getConnection();
        if (!conn) return result;

        std::string idQuery =
            "SELECT member_id FROM member WHERE login_id = ? AND role = 1";
        DBHelper::executeSelectOneString(conn, idQuery, {userId}, memberId);
        DBConnectionPool::getInstance().releaseConnection(conn);
    }

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return result;

    // orders 테이블 INSERT
    std::string orderQuery =
        "INSERT INTO orders "
        "(member_id, store_id, order_type, status, "
        " road_address, detail_address, total_price, delivery_fee, "
        " payment_method, store_request, rider_request) "
        "VALUES (?, ?, '배달', '주문대기', ?, ?, ?, ?, ?, ?, ?)";

    // member_id가 NULL이면 파라미터 처리
    std::vector<std::string> orderParams;
    if (memberId == "NULL" || memberId.empty()) {
        orderQuery =
            "INSERT INTO orders "
            "(store_id, order_type, status, "
            " road_address, detail_address, total_price, delivery_fee, "
            " payment_method, store_request, rider_request) "
            "VALUES (?, '배달', '주문대기', ?, ?, ?, ?, ?, ?, ?)";
        orderParams = {
            storeId, roadAddr, detailAddr,
            totalPrice, deliveryFee, payMethod, storeReq, riderReq
        };
    } else {
        orderParams = {
            memberId, storeId, roadAddr, detailAddr,
            totalPrice, deliveryFee, payMethod, storeReq, riderReq
        };
    }

    if (!DBHelper::executeUpdate(conn, orderQuery, orderParams)) {
        DBConnectionPool::getInstance().releaseConnection(conn);
        return result;
    }

    // 방금 삽입된 order_id 조회
    std::string orderId;
    DBHelper::executeSelectOneString(
        conn,
        "SELECT LAST_INSERT_ID()",
        {}, orderId);

    // order_items INSERT
    const auto &items = payload["items"];
    bool ok = true;
    for (const auto &item : items) {
        std::string menuId    = std::to_string(item["menuId"].get<int>());
        std::string menuName  = item["menuName"];
        std::string qty       = std::to_string(item["quantity"].get<int>());
        std::string unitPrice = std::to_string(item["unitPrice"].get<int>());

        ok = DBHelper::executeUpdate(conn,
            "INSERT INTO order_items "
            "(order_id, menu_id, menu_name, quantity, unit_price) "
            "VALUES (?, ?, ?, ?, ?)",
            {orderId, menuId, menuName, qty, unitPrice});
        if (!ok) break;
    }

    DBConnectionPool::getInstance().releaseConnection(conn);

    if (ok) {
        result["success"] = true;
        result["orderId"] = std::stoi(orderId);
        std::cout << "[ClientService] 주문 완료: orderId=" 
                  << orderId << "\n";
    }
    return result;
}

// ─────────────────────────────────────────
//  주문 내역 조회
// ─────────────────────────────────────────
std::vector<nlohmann::json> ClientService::processGetOrderList(
    const nlohmann::json &payload)
{
    std::vector<nlohmann::json> result;
    if (!payload.contains("userId")) return result;

    std::string userId = payload["userId"];

    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn) return result;

    std::string query =
        "SELECT o.order_id, o.store_id, s.store_name, "
        "       o.status, o.total_price, o.ordered_at "
        "FROM orders o "
        "JOIN store s ON o.store_id = s.store_id "
        "JOIN member m ON o.member_id = m.member_id "
        "WHERE m.login_id = ? AND m.role = 1 "
        "ORDER BY o.ordered_at DESC";

    std::vector<int> colSizes = {10, 10, 100, 20, 10, 20};
    std::vector<std::vector<std::string>> rows;

    if (DBHelper::executeSelectMultipleRows(
            conn, query, {userId}, colSizes, rows)) {
        for (const auto &row : rows) {
            nlohmann::json item;
            item["orderId"]    = std::stoi(row[0]);
            item["storeId"]    = std::stoi(row[1]);
            item["storeName"]  = row[2];
            item["status"]     = row[3];
            item["totalPrice"] = std::stoi(row[4]);
            item["orderedAt"]  = row[5];
            result.push_back(item);
        }
    }

    DBConnectionPool::getInstance().releaseConnection(conn);
    return result;
}

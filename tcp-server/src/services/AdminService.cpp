#include "services/AdminService.hpp"
#include "DBConnectionPool.h"
#include <mysql/mysql.h>
#include <iostream>

json AdminService::getUserById(int targetId)
{
    MYSQL *conn = DBConnectionPool::getInstance().getConnection();
    if (!conn)
        return {};

    // 1. 매핑표에 맞추어 테이블명(member)과 컬럼명 수정
    std::string q = "SELECT member_id, login_id, name, role, email, phone, is_banned, account_num "
                    "FROM member WHERE member_id = " +
                    std::to_string(targetId);

    if (mysql_query(conn, q.c_str()))
    {
        std::cerr << "[AdminService] getUserById 쿼리 실패: " << mysql_error(conn) << "\n";
        DBConnectionPool::getInstance().releaseConnection(conn);
        return {};
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);
    json result = {};

    // 2. 클라이언트가 요구하는 JSON 키(RES_...)로 정확히 매핑
    if (row)
    {
        result = {
            {"RES_member_id", row[0] ? std::stoi(row[0]) : 0},
            {"RES_login_id", row[1] ? row[1] : ""},
            {"RES_name", row[2] ? row[2] : ""},
            {"RES_role", row[3] ? row[3] : ""},
            {"RES_email", row[4] ? row[4] : ""},
            {"RES_phone", row[5] ? row[5] : ""},
            // is_banned는 DB에서 보통 TINYINT(0 또는 1)로 관리되므로 정수형으로 변환합니다.
            {"RES_is_banned", row[6] ? std::stoi(row[6]) : 0},
            {"RES_account_num", row[7] ? row[7] : ""}};
    }

    mysql_free_result(res);
    DBConnectionPool::getInstance().releaseConnection(conn);

    std::cout << "[1004 전송 JSON] " << result.dump() << "\n";

    return result;
}
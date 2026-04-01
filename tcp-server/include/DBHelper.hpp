#ifndef DB_HELPER_HPP
#define DB_HELPER_HPP

#include <mysql/mysql.h>
#include <string>
#include <vector>

class DBHelper
{
public:
    // INSERT, UPDATE, DELETE 용도 (데이터 변경)
    // 반환값: 쿼리 실행 성공 여부 (true/false)
    static bool executeUpdate(MYSQL *conn, const std::string &query, const std::vector<std::string> &params);

    // 단일 문자열 결과(예: 비밀번호 조회)를 가져오는 SELECT 용도
    // 반환값: 데이터를 성공적으로 찾았는지 여부 (true/false)
    // 찾은 결과는 resultStr에 담깁니다.
    static bool executeSelectOneString(MYSQL *conn, const std::string &query, const std::vector<std::string> &params, std::string &resultStr);

    // 여러 행 조회 — 각 행을 문자열 벡터로 반환
    static bool executeSelectMultipleRows(
        MYSQL *conn,
        const std::string &query,
        const std::vector<std::string> &params,
        const std::vector<int> &columnSizes,   // 각 컬럼 버퍼 크기
        std::vector<std::vector<std::string>> &results); // 결과 저장
};

#endif // DB_HELPER_HPP
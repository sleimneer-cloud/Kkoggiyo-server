#include "DBHelper.hpp"
#include <iostream>
#include <cstring> // memset 용도

bool DBHelper::executeUpdate(MYSQL *conn, const std::string &query, const std::vector<std::string> &params) // UPDATE, INSERT, DELETE 쿼리를 실행하는 함수
{
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt)
        return false;

    // 1. 쿼리 준비 (컴파일)
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) // 쿼리 준비 실패 시 에러 메시지 출력 후 종료
    {
        std::cerr << "[DBHelper] Prepare Error: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return false;
    }

    // 2. 파라미터 바인딩 준비. 파라미터 바인딩의 뜻은 쿼리에서 ?로 표시된 부분에 실제 값을 연결하는 것을 말한다.
    // 예를 들어, "SELECT * FROM users WHERE id = ?" 라는 쿼리가 있다면, ? 부분에 실제 id 값을 연결하는 것이다.
    // 파라미터 바인딩을 하는 이유는 SQL 인젝션 공격을 방지하고, 쿼리 실행 시 성능을 향상시키기 위해서이다.
    // 파라미터 바인딩을 통해 쿼리를 실행할 때마다 쿼리를 다시 컴파일할 필요 없이, 준비된 쿼리에 파라미터만 바꿔서 실행할 수 있기 때문에 성능이 향상된다.
    std::vector<MYSQL_BIND> binds(params.size());
    std::vector<unsigned long> lengths(params.size()); // 문자열 길이를 저장할 메모리 공간 유지 (매우 중요!)
    memset(binds.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (size_t i = 0; i < params.size(); ++i)
    {
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = (void *)params[i].c_str();
        lengths[i] = params[i].length();
        binds[i].buffer_length = lengths[i];
        binds[i].length = &lengths[i];
    }

    // 파라미터가 있을 때만 바인딩
    if (!params.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
        std::cerr << "[DBHelper] Bind Error: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return false;
    }

    // 3. 쿼리 실행
    if (mysql_stmt_execute(stmt) != 0)
    {
        std::cerr << "[DBHelper] Execute Error: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

bool DBHelper::executeSelectOneString(MYSQL *conn, const std::string &query, const std::vector<std::string> &params, std::string &resultStr)
// SELECT 쿼리를 실행하여 하나의 문자열 결과를 반환하는 함수
{
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt)
        return false;

    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0)
    {
        std::cerr << "[DBHelper] Prepare Error: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return false;
    }

    std::vector<MYSQL_BIND> binds(params.size());      // 파라미터 바인딩을 위한 벡터. 각 요소는 MYSQL_BIND 구조체로, 쿼리에서 ?로 표시된 부분에 실제 값을 연결하는 데 사용된다.
    std::vector<unsigned long> lengths(params.size()); // 문자열 길이를 저장할 메모리 공간 유지 (매우 중요!). 각 요소는 해당 파라미터의 문자열 길이를 저장하는 데 사용된다. MySQL C API에서 문자열 파라미터를 바인딩할 때, 각 문자열의 길이를 명시적으로 제공해야 하기 때문에 이 벡터가 필요하다.
    memset(binds.data(), 0, sizeof(MYSQL_BIND) * params.size());

    for (size_t i = 0; i < params.size(); ++i)
    {
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = (void *)params[i].c_str();
        lengths[i] = params[i].length();
        binds[i].buffer_length = lengths[i];
        binds[i].length = &lengths[i];
    }

    if (!params.empty() && mysql_stmt_bind_param(stmt, binds.data()) != 0)
    {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt) != 0)
    {
        mysql_stmt_close(stmt);
        return false;
    }

    // 4. 결과(Result)를 받을 바인딩 세팅 (비밀번호 하나만 받으므로 배열 크기는 1)
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    char result_buffer[256]; // 비밀번호가 들어갈 넉넉한 버퍼
    unsigned long result_length;
    my_bool is_null;
    my_bool error;

    result_bind[0].buffer_type = MYSQL_TYPE_STRING;
    result_bind[0].buffer = result_buffer;
    result_bind[0].buffer_length = sizeof(result_buffer);
    result_bind[0].length = &result_length;
    result_bind[0].is_null = &is_null;
    result_bind[0].error = &error;

    if (mysql_stmt_bind_result(stmt, result_bind) != 0)
    {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_store_result(stmt) != 0)
    {
        mysql_stmt_close(stmt);
        return false;
    }

    // 5. 한 줄 읽어오기
    bool found = false;
    if (mysql_stmt_fetch(stmt) == 0)
    { // 데이터를 성공적으로 가져왔다면
        if (!is_null)
        {
            resultStr = std::string(result_buffer, result_length);
            found = true;
        }
    }

    mysql_stmt_free_result(stmt);
    mysql_stmt_close(stmt);
    return found;
}
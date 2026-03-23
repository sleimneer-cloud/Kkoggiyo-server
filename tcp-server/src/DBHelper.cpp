#include "DBHelper.hpp"
#include <iostream>
#include <cstring> // memset 용도

bool DBHelper::executeUpdate(MYSQL *conn, const std::string &query, const std::vector<std::string> &params)
{
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt)
        return false;

    // 1. 쿼리 준비 (컴파일)
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0)
    {
        std::cerr << "[DBHelper] Prepare Error: " << mysql_stmt_error(stmt) << "\n";
        mysql_stmt_close(stmt);
        return false;
    }

    // 2. 파라미터 바인딩 준비
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

    std::vector<MYSQL_BIND> binds(params.size());
    std::vector<unsigned long> lengths(params.size());
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
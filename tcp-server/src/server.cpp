#include <iostream>
#include "net/ConnectionManager.hpp"
#include "router/MessageRouter.hpp"
#include "DBConnectionPool.h"

static constexpr int PORT = 9000;

int main()
{
    try
    {
        DBConnectionPool::getInstance().init("10.10.10.124", "DEV", "1234", "delivery", 3306, 10);
        // DB 커넥션 풀 초기화 (호스트, 사용자, 비밀번호, DB 이름, 포트, 풀 크기)
        MessageRouter router;
        ConnectionManager server(PORT, router);
        server.run();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "[치명적 에러] " << e.what() << "\n";
        return 1;
    }
}
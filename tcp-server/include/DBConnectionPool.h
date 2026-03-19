#pragma once

#include <mysql/mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

class DBConnectionPool
{
public:
    // 싱글톤 패턴: 어디서든 동일한 하나의 풀에 접근하도록 함
    static DBConnectionPool &getInstance();

    // 풀 초기화 (서버 켤 때 한 번만 호출)
    void init(const std::string &host, const std::string &user, const std::string &pass,
              const std::string &db, int port, int poolSize);

    // 연결 통로 빌려오기
    MYSQL *getConnection();

    // 다 쓴 통로 반납하기
    void releaseConnection(MYSQL *conn);

private:
    // 외부에서 함부로 생성하지 못하게 막음 (싱글톤)
    DBConnectionPool() = default;
    ~DBConnectionPool();
    DBConnectionPool(const DBConnectionPool &) = delete;
    DBConnectionPool &operator=(const DBConnectionPool &) = delete;

    std::queue<MYSQL *> connectionQueue_; // 연결들을 보관할 큐(대기열)
    std::mutex mutex_;                    // 동시에 꺼내가지 못하게 막는 자물쇠
    std::condition_variable condVar_;     // 통로가 다 떨어졌을 때 기다리게 하는 알람
};
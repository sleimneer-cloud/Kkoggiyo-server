#include "DBConnectionPool.h"
#include <iostream>

// 유일한 객체를 반환하는 함수 (싱글톤 패턴으로 구현)
DBConnectionPool &DBConnectionPool::getInstance()
{
    static DBConnectionPool instance;
    return instance;
}

// 1. 초기화: 서버가 켜질 때 지정된 갯수(poolSize)만큼 미리 DB와 연결해둠
void DBConnectionPool::init(const std::string &host, const std::string &user, const std::string &pass,
                            const std::string &db, int port, int poolSize)
{
    for (int i = 0; i < poolSize; ++i)
    {
        MYSQL *conn = mysql_init(NULL);
        if (conn == NULL)
        {
            std::cerr << "[DB Pool] mysql_init 실패\n";
            continue;
        }

        bool reconnect = true;                                // "재연결 기능을 켜겠다(true)"라는 스위치
        mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect); // 자동 재연결 옵션 설정

        // 실제 DB에 연결 (delivery DB)
        if (mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, NULL, 0) == NULL)
        {
            std::cerr << "[DB Pool 에러] " << mysql_error(conn) << "\n";
            mysql_close(conn);
            continue;
        }

        // 연결 성공 시 큐에 보관
        connectionQueue_.push(conn);
    }
    std::cout << "[DB Pool] 초기화 완료. 생성된 커넥션 수: " << connectionQueue_.size() << "\n";
}

// 2. 통로 빌려주기
MYSQL *DBConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_); // 자물쇠 잠금! (다른 스레드 접근 대기)

    // 만약 큐가 비어있다면(모두 사용 중이라면) 누군가 반납할 때까지 여기서 대기함
    condVar_.wait(lock, [this]()
                  { return !connectionQueue_.empty(); });

    // 큐에서 하나 꺼내기
    MYSQL *conn = connectionQueue_.front();
    connectionQueue_.pop();

    // 통로가 살아있는지 확인 : mysql_ping() 함수. 이 함수는 연결이 유효한지 확인하고, 끊어진 경우 자동으로 재연결을 시도함.
    if (mysql_ping(conn) != 0) // mysql_ping() 함수로 상태 체크
    {                          // 자동 재연결 옵션이 발동해서 어떻게든 다시 연결해 보려고 발악을 했지만, 결국 실패한 경우
        std::cerr << "[DB Pool] 치명적 오류: DB 서버가 응답하지 않습니다.\n";
        // 문제가 있는 연결은 닫아버리고 NULL 반환 (이 경우 서비스 계층에서 로그인 실패로 처리할 수 있도록)
        mysql_close(conn);
        return NULL;
    }

    return conn; // 자물쇠는 함수가 끝나면서 자동으로 풀림
}

// 3. 다 쓴 통로 반납받기
void DBConnectionPool::releaseConnection(MYSQL *conn)
{
    if (conn == NULL)
        return;

    {
        std::lock_guard<std::mutex> lock(mutex_); // 자물쇠 잠금!
        connectionQueue_.push(conn);              // 다시 큐에 집어넣음
    }

    // 기다리고 있는 스레드가 있다면 반납되었다고 알람을 울려줌
    condVar_.notify_one();
}

// 4. 소멸자: 서버가 꺼질 때 모든 연결을 안전하게 닫아줌
DBConnectionPool::~DBConnectionPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!connectionQueue_.empty())
    {
        MYSQL *conn = connectionQueue_.front();
        connectionQueue_.pop();
        mysql_close(conn);
    }
    std::cout << "[DB Pool] 모든 커넥션 안전하게 종료됨.\n";
}
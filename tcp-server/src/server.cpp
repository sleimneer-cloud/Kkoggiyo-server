#include <iostream>
#include <cstdlib> // std::getenv
#include <stdexcept>
#include "net/ConnectionManager.hpp"
#include "router/MessageRouter.hpp"
#include "DBConnectionPool.h"
#include <csignal>

static constexpr int PORT = 9000;

// .env 파일을 읽어 환경변수로 로드하는 함수
// KEY=VALUE 형식만 처리, # 주석과 빈 줄은 무시
static void loadEnv(const std::string &path)
{
    FILE *f = fopen(path.c_str(), "r");
    if (!f)
    {
        std::cerr << "[설정] .env 파일 없음 (" << path << ") — 시스템 환경변수를 사용합니다.\n";
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f))
    {
        // 줄 끝 개행 제거
        std::string s(line);
        if (!s.empty() && s.back() == '\n')
            s.pop_back();
        if (!s.empty() && s.back() == '\r')
            s.pop_back();

        // 빈 줄, 주석 무시
        if (s.empty() || s[0] == '#')
            continue;

        // KEY=VALUE 분리
        auto eq = s.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = s.substr(0, eq);
        std::string val = s.substr(eq + 1);

        // 이미 설정된 환경변수는 덮어쓰지 않음 (시스템 환경변수 우선)
        if (!std::getenv(key.c_str()))
            setenv(key.c_str(), val.c_str(), 0);
    }
    fclose(f);
}

// 환경변수 읽기 — 없으면 즉시 예외 (필수값 누락 시 서버 시작 차단)
static std::string requireEnv(const char *key)
{
    const char *val = std::getenv(key);
    if (!val || std::string(val).empty())
        throw std::runtime_error(std::string("필수 환경변수 누락: ") + key);
    return std::string(val);
}

int main()
{
    // + 추가: SIGPIPE 시그널 무시 (클라이언트 강제 종료 시 서버 즉사 방지)
    signal(SIGPIPE, SIG_IGN);
    try
    {
        // .env 로드 (server 실행 위치 기준)
        loadEnv(".env");

        // 환경변수에서 DB 설정 읽기
        std::string dbHost = requireEnv("DB_HOST");
        std::string dbUser = requireEnv("DB_USER");
        std::string dbPass = requireEnv("DB_PASS");
        std::string dbName = requireEnv("DB_NAME");
        int dbPort = std::stoi(requireEnv("DB_PORT"));
        int dbPoolSize = std::stoi(requireEnv("DB_POOL_SIZE"));

        DBConnectionPool::getInstance().init(dbHost, dbUser, dbPass, dbName, dbPort, dbPoolSize);
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

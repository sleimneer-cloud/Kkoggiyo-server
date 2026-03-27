#pragma once
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>
#include <iostream>

// 유저 한 명의 상태를 기억할 구조체
struct Session
{
    int fd;
    int clientType; // 0: 고객, 1: 사장님, 2: 라이더, 3: 관리자
    std::string userId;
};

// 접속자를 관리하는 명부 클래스
class SessionManager
{
private:
    std::unordered_map<int, Session> activeSessions;
    std::mutex mtx;

    // [싱글톤 핵심 1] 생성자를 private으로 숨겨서 밖에서 마음대로 생성하지 못하게 함
    SessionManager() = default;

public:
    // [싱글톤 핵심 2] 복사 금지
    SessionManager(const SessionManager &) = delete;
    SessionManager &operator=(const SessionManager &) = delete;

    // [싱글톤 핵심 3] 오직 이 함수를 통해서만 유일한 명부(인스턴스)에 접근 가능
    static SessionManager &getInstance()
    {
        static SessionManager instance;
        return instance;
    }

    // 기능 선언
    void addSession(int fd, int type, const std::string &id);
    void removeSession(int fd);
    std::vector<int> getAdminFds();
    std::vector<int> getRiderFds();
    int getFdByUserId(const std::string &targetId);
    std::string getUserIdByFd(int fd);
};
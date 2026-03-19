#include "SessionManager.hpp"

void SessionManager::addSession(int fd, int type, const std::string &id)
{
    std::lock_guard<std::mutex> lock(mtx);
    activeSessions[fd] = {fd, type, id};
    std::cout << "[명부 등록] FD: " << fd << " / ID: " << id << " / Type: " << type << " 등록 완료!\n";
}

void SessionManager::removeSession(int fd)
{
    std::lock_guard<std::mutex> lock(mtx);
    if (activeSessions.erase(fd))
    {
        std::cout << "[명부 삭제] FD: " << fd << " 접속 해제됨.\n";
    }
}

std::vector<int> SessionManager::getAdminFds()
{
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<int> admins;
    for (const auto &pair : activeSessions)
    {
        if (pair.second.clientType == 0)
        { // 0번이 관리자
            admins.push_back(pair.first);
        }
    }
    return admins;
}

int SessionManager::getFdByUserId(const std::string &targetId)
{
    std::lock_guard<std::mutex> lock(mtx);
    for (const auto &pair : activeSessions)
    {
        if (pair.second.userId == targetId)
        {
            return pair.first;
        }
    }
    return -1; // 못 찾으면 -1 반환
}
#include "repo/InMemoryUserRepository.hpp"

std::optional<UserRecord> InMemoryUserRepository::findById(const std::string &userId) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = users_.find(userId);
    if (it == users_.end())
        return std::nullopt;
    return it->second;
}

void InMemoryUserRepository::upsert(const UserRecord &user)
{
    std::lock_guard<std::mutex> lock(mtx_);
    users_[user.userId] = user;
}


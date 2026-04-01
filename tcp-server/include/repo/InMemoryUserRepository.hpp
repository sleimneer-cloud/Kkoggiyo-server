#pragma once

#include <unordered_map>
#include <mutex>

#include "repo/IUserRepository.hpp"

class InMemoryUserRepository final : public IUserRepository
{
public:
    std::optional<UserRecord> findById(const std::string &userId) const override;
    void upsert(const UserRecord &user) override;

private:
    mutable std::mutex mtx_;
    std::unordered_map<std::string, UserRecord> users_;
};


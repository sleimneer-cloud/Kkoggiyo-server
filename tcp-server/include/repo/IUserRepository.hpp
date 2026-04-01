#pragma once

#include <optional>
#include <string>

// DB 연동을 대비한 사용자 레코드/저장소 인터페이스 (현재 서버 로직에 직접 연결하진 않음)
struct UserRecord
{
    std::string userId;
    std::string userPw;
    int clientType{0};
    bool approved{true};
};

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;
    virtual std::optional<UserRecord> findById(const std::string &userId) const = 0;
    virtual void upsert(const UserRecord &user) = 0;
};


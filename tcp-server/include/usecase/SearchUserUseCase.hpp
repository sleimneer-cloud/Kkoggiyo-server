#pragma once

#include "json.hpp"
// #include "services/AdminService.hpp" // 이 줄은 지우거나 주석 처리해도 됩니다.

using json = nlohmann::json;

class AdminService; // ← 추가 (전방 선언: "AdminService라는 클래스가 있을 거야"라고 미리 알려줌)

class SearchUserUseCase
{
public:
    explicit SearchUserUseCase(AdminService &adminSvc);
    ~SearchUserUseCase() = default;

    json execute(int targetId);

private:
    AdminService &adminSvc_;
};
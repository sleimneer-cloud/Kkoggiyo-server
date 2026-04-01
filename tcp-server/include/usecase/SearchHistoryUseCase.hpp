#pragma once

#include "json.hpp"
#include "services/AdminService.hpp"
#include <string>

using json = nlohmann::json;

class SearchHistoryUseCase
{
public:
    explicit SearchHistoryUseCase(AdminService &adminSvc);
    ~SearchHistoryUseCase() = default;

    json execute(int targetId);
    json execute(const std::string& loginId);

private:
    AdminService &adminSvc_;
};

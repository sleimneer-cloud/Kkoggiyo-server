#include "usecase/SearchHistoryUseCase.hpp"

SearchHistoryUseCase::SearchHistoryUseCase(AdminService &adminSvc)
    : adminSvc_(adminSvc)
{
}

json SearchHistoryUseCase::execute(int targetId)
{
    return adminSvc_.getOrderHistoryByTargetId(targetId);
}

json SearchHistoryUseCase::execute(const std::string& loginId)
{
    return adminSvc_.getOrderHistoryByLoginId(loginId);
}

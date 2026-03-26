#pragma once

#include "json.hpp"
#include "services/AdminService.hpp"
#include "usecase/SearchUserUseCase.hpp"
// #include "usecase/SearchHistoryUseCase.hpp" // 나중에 추가될 유스케이스

using json = nlohmann::json;

class AdminHandler
{
public:
    AdminHandler();
    ~AdminHandler() = default;

    // 라우터에서 직접 호출할 핸들러 메서드들
    void handleSearchId(int clientFd, const json &j);
    void handleSearchHistory(int clientFd, const json &j); // MessageRouter에 이미 정의된 처리용

private:
    // 1. 사용할 서비스 인스턴스들
    AdminService adminSvc_;

    // 2. 서비스 인스턴스를 주입받아 동작할 유스케이스들
    SearchUserUseCase searchUserUC_;
    // SearchHistoryUseCase searchHistoryUC_; // 나중에 기능 추가 시 활성화
};
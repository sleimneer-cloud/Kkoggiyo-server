#pragma once
#include <string>

class StoreService
{
public:
    StoreService() = default;
    ~StoreService() = default;

    // 가게 오픈/마감 상태 토글 메서드
    // 로그인 ID를 기반으로 해당 사장님의 가게 상태를 변경합니다.
    bool toggleStoreStatus(const std::string& loginId, int isOpen, std::string& outMsg);
};

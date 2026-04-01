#include <string>
#include "json.hpp"

using json = nlohmann::json;

class StoreService
{
public:
    StoreService() = default;
    ~StoreService() = default;

    // 가게 오픈/마감 상태 토글 메서드
    // 특정 가게 ID와 사장님 ID를 검증하여 상태를 변경합니다.
    bool toggleStoreStatus(const std::string& loginId, int storeId, int isOpen, std::string& outMsg);

    bool registerStore(const std::string& loginId, const std::string& name, 
                       const std::string& addr, const std::string& regNum, 
                       int categoryId, std::string& outMsg);

    // 사장님의 모든 가게 목록 조회
    json getOwnerStores(const std::string& loginId);
};

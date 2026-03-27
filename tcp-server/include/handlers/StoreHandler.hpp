#pragma once
#include "json.hpp"
#include "services/StoreService.hpp"

using json = nlohmann::json;

class StoreHandler
{
public:
    StoreHandler() = default;
    ~StoreHandler() = default;

    // CS_STORE_OPEN_REQ (4910) 패킷 수신 처리
    void handleStoreOpenReq(int clientFd, const json &j);

private:
    StoreService storeSvc_;
};

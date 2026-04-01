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

    // CS_STORE_REGISTER_REQ (4940) 패킷 수신 처리
    void handleStoreRegister(int clientFd, const json &j);

    // CS_OWNER_STORE_LIST_REQ (4960) 패킷 수신 처리
    void handleOwnerStoreList(int clientFd, const json &j);

private:
    StoreService storeSvc_;
};

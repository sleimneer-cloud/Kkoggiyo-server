#pragma once

#include <cstdint>
#include <vector>

// 클라이언트 fd마다 유지하는 상태(현재는 수신 누적 버퍼만 필요)
struct ConnectionState
{
    std::vector<uint8_t> recvBuffer;
};


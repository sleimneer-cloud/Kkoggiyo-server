#pragma once

#include <cstddef>

namespace SocketIO
{
    // write()가 partial write를 할 수 있으므로, 끝까지 보내는 유틸.
    // non-blocking에서 EAGAIN이 오면 false를 반환(간단 정책).
    bool writeAll(int fd, const void *data, size_t len);
}


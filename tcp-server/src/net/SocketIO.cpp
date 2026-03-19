#include "net/SocketIO.hpp"

#include <unistd.h>
#include <errno.h>

namespace SocketIO
{
    bool writeAll(int fd, const void *data, size_t len)
    {
        const char *p = static_cast<const char *>(data);
        size_t remaining = len;
        while (remaining > 0)
        {
            ssize_t n = write(fd, p, remaining);
            if (n > 0)
            {
                p += n;
                remaining -= static_cast<size_t>(n);
                continue;
            }
            if (n == 0)
            {
                return false;
            }
            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return false;
            }
            return false;
        }
        return true;
    }
}


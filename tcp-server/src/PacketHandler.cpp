#include "PacketHandler.hpp"
#include <iostream>
#include "net/SocketIO.hpp"

std::unordered_map<int, std::unique_ptr<std::mutex>> PacketHandler::fdMutexMap_;
std::mutex PacketHandler::mapMutex_;

std::mutex &PacketHandler::getFdMutex(int fd)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    auto it = fdMutexMap_.find(fd);
    if (it == fdMutexMap_.end())
        fdMutexMap_[fd] = std::make_unique<std::mutex>();
    return *fdMutexMap_[fd];
}

void PacketHandler::sendPacket(int target_fd, PacketType type, const json &payload)
{
    json msg = payload;
    msg["type"] = static_cast<int32_t>(type);
    std::string jsonStr = msg.dump();
    int32_t len = static_cast<int32_t>(jsonStr.size());

    std::lock_guard<std::mutex> lock(getFdMutex(target_fd));

    bool ok = true;
    ok = ok && SocketIO::writeAll(target_fd, &len, sizeof(int32_t));
    ok = ok && SocketIO::writeAll(target_fd, jsonStr.data(), jsonStr.size());

    if (ok)
        std::cout << "[송신] FD: " << target_fd << " / 타입: " << (int)type << " 전송 완료.\n";
    else
        std::cout << "[경고] 송신 실패. FD: " << target_fd << " / 타입: " << (int)type << "\n";
}
#pragma once
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include "protocol.hpp"
#include "json.hpp"
#include "SessionManager.hpp"

using json = nlohmann::json;

class PacketHandler
{
public:
    static void sendPacket(int target_fd, PacketType type, const json &payload);
    static void removeFd(int fd);

private:
    static std::mutex &getFdMutex(int fd);

    static std::unordered_map<int, std::unique_ptr<std::mutex>> fdMutexMap_;
    static std::mutex mapMutex_;
};
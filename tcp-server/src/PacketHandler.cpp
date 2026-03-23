#include "PacketHandler.hpp"
#include <iostream>
#include <arpa/inet.h> // ← 추가
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
    int32_t len = htonl(static_cast<int32_t>(jsonStr.size())); // ← htonl 추가

    std::lock_guard<std::mutex> lock(getFdMutex(target_fd)); // 해당 FD에 대한 mutex 잠금.
    // 이유는 송신 과정에서 여러 스레드가 동시에 같은 FD에 접근하여 데이터를 송신할 수 있기 때문입니다.

    bool ok = true;
    ok = ok && SocketIO::writeAll(target_fd, &len, sizeof(int32_t));
    ok = ok && SocketIO::writeAll(target_fd, jsonStr.data(), jsonStr.size());

    if (ok)
        std::cout << "[송신] FD: " << target_fd << " / 타입: " << (int)type << " 전송 완료.\n";
    else
        std::cout << "[경고] 송신 실패. FD: " << target_fd << " / 타입: " << (int)type << "\n";
}

void PacketHandler::removeFd(int fd) // FD가 닫힐 때 호출되어야 함. 이걸 사용하는 이유는
// FD가 닫힐 때 해당 FD에 대한 mutex를 정리하여 메모리 누수를 방지하기 위함입니다.
// FD가 닫히면 더 이상 해당 FD에 대한 송수신이 이루어지지 않으므로, 관련된 mutex도 필요 없어집니다.
// 따라서 removeFd 함수를 통해 해당 FD의 mutex를 제거하여 자원을 효율적으로 관리할 수 있습니다.
{
    std::lock_guard<std::mutex> lock(mapMutex_);                        // mapMutex_ 잠금
    fdMutexMap_.erase(fd);                                              // 해당 FD의 mutex 제거
    std::cout << "[PacketHandler] FD: " << fd << " mutex 정리 완료.\n"; // 로그 추가
}
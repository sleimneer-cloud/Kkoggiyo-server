#pragma once
#include "json.hpp"
#include "protocol.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>

// WorkItem: 처리할 패킷 하나를 담는 구조체
struct WorkItem
{
    int client_fd;
    std::string jsonStr; // 패킷의 JSON 문자열
};

// jsonStr에서 type 필드를 안전하게 추출하는 헬퍼 함수
static inline int extractPacketType(const std::string &jsonStr)
{
    if (jsonStr.empty())
        return -1;
    try
    {
        return nlohmann::json::parse(jsonStr).value("type", -1);
    }
    catch (...)
    {
        return -1;
    }
}

class WorkQueue
{
public:
    explicit WorkQueue(int workerCount, std::function<void(WorkItem)> handler)
        : stop_(false), handler_(std::move(handler)),
          totalPushed_(0), totalProcessed_(0)
    {
        std::cout << "[WorkQueue] 초기화 시작. 요청 Worker 수: " << workerCount << "\n";

        for (int i = 0; i < workerCount; ++i)
        {
            workers_.emplace_back([this, i]
                                  {
                std::cout << "[WorkQueue] Worker #" << i << " 스레드 시작. TID: "
                          << std::this_thread::get_id() << "\n";
                workerLoop(i);
                std::cout << "[WorkQueue] Worker #" << i << " 스레드 종료.\n"; });
        }

        std::cout << "[WorkQueue] 초기화 완료. 실제 생성된 Worker 수: "
                  << workers_.size() << "\n";
    }

    ~WorkQueue()
    {
        std::cout << "[WorkQueue] 종료 시작. 처리 대기 중인 항목: "
                  << queue_.size() << "\n";
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto &t : workers_)
        {
            if (t.joinable())
                t.join();
        }
        std::cout << "[WorkQueue] 종료 완료. 총 push: " << totalPushed_
                  << " / 총 처리: " << totalProcessed_ << "\n";
    }

    void push(WorkItem item)
    {
        // move 전에 로그에 필요한 값을 미리 추출
        int client_fd = item.client_fd;
        int msgType = extractPacketType(item.jsonStr);

        {
            std::lock_guard<std::mutex> lock(mtx_);
            queue_.push(std::move(item)); // 여기서 item 내용이 비워짐
            ++totalPushed_;

            size_t qSize = queue_.size();
            if (qSize % 10 == 0 && qSize > 0)
            {
                std::cout << "[WorkQueue] ⚠️  현재 큐 적체: " << qSize
                          << "개 (총 push: " << totalPushed_ << ")\n";
            }
            else
            {
                // move 전에 뽑아둔 값으로 로그 출력
                std::cout << "[WorkQueue] push 완료. FD: " << client_fd
                          << " / PacketType: " << msgType
                          << " / 현재 큐 크기: " << qSize << "\n";
            }
        }
        cv_.notify_one();
    }

    void printStatus() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << "[WorkQueue 상태] "
                  << "Worker 수: " << workers_.size()
                  << " / 큐 적체: " << queue_.size()
                  << " / 총 push: " << totalPushed_
                  << " / 총 처리: " << totalProcessed_
                  << "\n";
    }

    size_t workerCount() const { return workers_.size(); }
    size_t queueSize() const
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

private:
    void workerLoop(int workerId)
    {
        while (true)
        {
            WorkItem item;
            {
                std::unique_lock<std::mutex> lock(mtx_);
                cv_.wait(lock, [this]
                         { return !queue_.empty() || stop_; });

                if (stop_ && queue_.empty())
                    return;

                item = std::move(queue_.front());
                queue_.pop();
            }

            // handler_ 호출 전에 로그에 필요한 값을 미리 추출
            int client_fd = item.client_fd;
            int msgType = extractPacketType(item.jsonStr);
            std::string jsonPreview = item.jsonStr.size() > 60
                                          ? item.jsonStr.substr(0, 60) + "..."
                                          : item.jsonStr;

            std::cout << "[WorkQueue] Worker #" << workerId
                      << " 처리 시작. FD: " << client_fd
                      << " / PacketType: " << msgType
                      << " / JSON: " << jsonPreview << "\n";

            try
            {
                handler_(std::move(item)); // 여기서 item 내용이 비워짐
                ++totalProcessed_;
                std::cout << "[WorkQueue] Worker #" << workerId
                          << " 처리 완료. 총 처리: " << totalProcessed_ << "\n";
            }
            catch (const std::exception &e)
            {
                std::cerr << "[WorkQueue] Worker #" << workerId
                          << " 처리 중 예외 발생: " << e.what() << "\n";
            }
        }
    }

    std::queue<WorkItem> queue_;
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stop_;
    std::function<void(WorkItem)> handler_;

    std::atomic<int> totalPushed_;
    std::atomic<int> totalProcessed_;
};
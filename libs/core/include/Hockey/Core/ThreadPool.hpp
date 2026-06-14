#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
namespace Hockey {
class ThreadPool {
public:
    ThreadPool() = default;
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void Start(uint32_t workerCount);
    void Stop();

    void Submit(std::function<void()> job);
    void WaitIdle();

    uint32_t WorkerCount() const;

private:
    void WorkerLoop();

    std::vector<std::thread> m_Workers;
    std::queue<std::function<void()>> m_Jobs;

    mutable std::mutex m_Mutex;
    std::condition_variable m_JobAvailable;
    std::condition_variable m_JobFinished;

    std::atomic_bool m_Running{false};
    uint32_t m_WorkerCount = 0;
    uint64_t m_PendingJobs = 0;
};
}

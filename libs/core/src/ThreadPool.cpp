#include "Hockey/Core/ThreadPool.hpp"
#include <exception>
#include <utility>
#include "Hockey/Core/Log.hpp"
namespace Hockey {
ThreadPool::~ThreadPool() { Stop(); }
void ThreadPool::Start(uint32_t workerCount) {
    if (m_Running.load()) {
        return;
    }
    m_WorkerCount = workerCount == 0 ? 1 : workerCount;
    m_PendingJobs = 0;
    m_Running.store(true);
    m_Workers.reserve(m_WorkerCount);
    for (uint32_t i = 0; i < m_WorkerCount; ++i) {
        m_Workers.emplace_back([this] { WorkerLoop(); });
    }
}
void ThreadPool::Stop() {
    if (!m_Running.exchange(false)) {
        return;
    }
    m_JobAvailable.notify_all();
    for (std::thread& worker : m_Workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_Workers.clear();
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::queue<std::function<void()>> empty;
    m_Jobs.swap(empty);
    m_PendingJobs = 0;
    m_WorkerCount = 0;
}
void ThreadPool::Submit(std::function<void()> job) {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Jobs.push(std::move(job));
        ++m_PendingJobs;
    }
    m_JobAvailable.notify_one();
}
void ThreadPool::WaitIdle() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_JobFinished.wait(lock, [this] { return m_PendingJobs == 0; });
}
uint32_t ThreadPool::WorkerCount() const { return m_WorkerCount; }
void ThreadPool::WorkerLoop() {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_JobAvailable.wait(lock, [this] { return !m_Running.load() || !m_Jobs.empty(); });
            if (!m_Running.load() && m_Jobs.empty()) {
                return;
            }
            job = std::move(m_Jobs.front());
            m_Jobs.pop();
        }
        try {
            job();
        } catch (const std::exception& error) {
            if (auto logger = Log::Core()) {
                logger->error("ThreadPool job threw exception: {}", error.what());
            }
        } catch (...) {
            if (auto logger = Log::Core()) {
                logger->error("ThreadPool job threw non-standard exception");
            }
        }
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            --m_PendingJobs;
            if (m_PendingJobs == 0) {
                m_JobFinished.notify_all();
            }
        }
    }
}
}

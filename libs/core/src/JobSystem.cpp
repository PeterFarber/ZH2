#include "Hockey/Core/JobSystem.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/ThreadPool.hpp"
namespace Hockey {
namespace {
std::unique_ptr<ThreadPool> g_Pool;
}
void JobSystem::Init(uint32_t workerCount) {
    if (g_Pool) {
        return;
    }
    uint32_t count = workerCount;
    if (count == 0) {
        const uint32_t hardware = Platform::HardwareThreadCount();
        count = hardware > 1 ? hardware - 1 : 1;
    }
    g_Pool = std::make_unique<ThreadPool>();
    g_Pool->Start(count);
}
void JobSystem::Shutdown() {
    if (g_Pool) {
        g_Pool->Stop();
        g_Pool.reset();
    }
}
void JobSystem::Submit(std::function<void()> job) {
    if (g_Pool) {
        g_Pool->Submit(std::move(job));
    } else {
        job();
    }
}
void JobSystem::WaitIdle() {
    if (g_Pool) {
        g_Pool->WaitIdle();
    }
}
void JobSystem::ParallelFor(size_t count, std::function<void(size_t)> fn) {
    if (count == 0) {
        return;
    }
    if (!g_Pool) {
        for (size_t i = 0; i < count; ++i) {
            fn(i);
        }
        return;
    }
    auto remaining = std::make_shared<std::atomic<size_t>>(count);
    auto mutex = std::make_shared<std::mutex>();
    auto finished = std::make_shared<std::condition_variable>();
    for (size_t i = 0; i < count; ++i) {
        g_Pool->Submit([i, fn, remaining, mutex, finished] {
            fn(i);
            if (remaining->fetch_sub(1) == 1) {
                std::lock_guard<std::mutex> lock(*mutex);
                finished->notify_all();
            }
        });
    }
    std::unique_lock<std::mutex> lock(*mutex);
    finished->wait(lock, [&] { return remaining->load() == 0; });
}
uint32_t JobSystem::WorkerCount() { return g_Pool ? g_Pool->WorkerCount() : 0; }
}

#pragma once
#include <functional>
#include <cstddef>
#include <cstdint>
namespace Hockey { class JobSystem { public: static void Init(uint32_t workerCount=0); static void Shutdown(); static void Submit(std::function<void()> job); static void WaitIdle(); static void ParallelFor(size_t count, std::function<void(size_t)> fn); static uint32_t WorkerCount(); }; }

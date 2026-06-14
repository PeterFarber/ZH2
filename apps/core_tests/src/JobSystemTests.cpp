#include "Test.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include <atomic>
#include <cstddef>
#include <vector>
using namespace Hockey;
void RunJobSystemTests() {
    HockeyTest::BeginSuite("JobSystemTests");

    JobSystem::Init(4);
    HK_CHECK(JobSystem::WorkerCount() >= 1);

    std::atomic<int> counter{0};
    for (int i = 0; i < 1000; ++i) {
        JobSystem::Submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    JobSystem::WaitIdle();
    HK_CHECK_EQ(counter.load(), 1000);

    const size_t indexCount = 10000;
    std::vector<int> visited(indexCount, 0);
    JobSystem::ParallelFor(indexCount, [&visited](size_t i) { visited[i] += 1; });
    bool everyIndexOnce = true;
    for (size_t i = 0; i < indexCount; ++i) {
        if (visited[i] != 1) {
            everyIndexOnce = false;
            break;
        }
    }
    HK_CHECK_MSG(everyIndexOnce, "ParallelFor visits every index exactly once");

    JobSystem::Shutdown();
    HK_CHECK_EQ(JobSystem::WorkerCount(), static_cast<uint32_t>(0));
}

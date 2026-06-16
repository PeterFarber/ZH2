#include "Test.hpp"

#include "Hockey/Core/ThreadPool.hpp"

#include <atomic>

using namespace Hockey;

void RunThreadPoolTests() {
    HockeyTest::BeginSuite("ThreadPoolTests");

    // Direct ThreadPool exercise (JobSystem is tested separately and only uses
    // the pool indirectly).
    ThreadPool pool;
    pool.Start(4);
    HK_CHECK_EQ(pool.WorkerCount(), static_cast<uint32_t>(4));

    std::atomic<int> counter{0};
    for (int i = 0; i < 2000; ++i) {
        pool.Submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
    }
    pool.WaitIdle();
    HK_CHECK_EQ(counter.load(), 2000);

    // WaitIdle with no outstanding jobs must return immediately.
    pool.WaitIdle();
    HockeyTest::RecordPass();

    pool.Stop();
    HK_CHECK_EQ(pool.WorkerCount(), static_cast<uint32_t>(0));

    // A worker count of zero is clamped to one running worker.
    ThreadPool single;
    single.Start(0);
    HK_CHECK_EQ(single.WorkerCount(), static_cast<uint32_t>(1));
    std::atomic<int> ran{0};
    single.Submit([&ran] { ran.fetch_add(1, std::memory_order_relaxed); });
    single.WaitIdle();
    HK_CHECK_EQ(ran.load(), 1);
    single.Stop();
}

#include "Test.hpp"

#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/SignalHandler.hpp"
#include "Hockey/Core/Timer.hpp"

#include <filesystem>

using namespace Hockey;

void RunPlatformTests() {
    HockeyTest::BeginSuite("PlatformTests");

    // Platform query surface.
    HK_CHECK(Platform::HardwareThreadCount() >= 1);
    HK_CHECK(!Platform::OSName().empty());
    HK_CHECK(!Platform::ExecutablePath().empty());
    HK_CHECK(std::filesystem::exists(Platform::ExecutablePath()));
    HK_CHECK(!Platform::CurrentWorkingDirectory().empty());

    // IsDebuggerAttached just needs to be callable and return a bool without
    // crashing; its value depends on the environment so it is not asserted.
    const bool debugger = Platform::IsDebuggerAttached();
    HK_CHECK(debugger == true || debugger == false);

    // Sleep should block for at least roughly the requested duration. Use a
    // generous lower bound to stay robust against scheduler jitter.
    {
        Timer timer;
        Platform::SleepMilliseconds(20);
        HK_CHECK_MSG(timer.ElapsedMilliseconds() >= 10.0, "SleepMilliseconds blocks for at least ~half the request");
    }

    // YieldThread must be callable without side effects.
    Platform::YieldThread();
    HockeyTest::RecordPass();
}

void RunTimerTests() {
    HockeyTest::BeginSuite("TimerTests");

    Timer timer;
    HK_CHECK(timer.ElapsedSeconds() >= 0.0);

    Platform::SleepMilliseconds(5);
    const double afterSleep = timer.ElapsedMilliseconds();
    HK_CHECK_MSG(afterSleep >= 1.0, "Timer advances after sleeping");

    timer.Reset();
    HK_CHECK_MSG(timer.ElapsedMilliseconds() <= afterSleep, "Reset() restarts the timer");
}

void RunSignalHandlerTests() {
    HockeyTest::BeginSuite("SignalHandlerTests");

    // Install is idempotent and must not raise. Raising real SIGINT/SIGTERM in
    // a test runner is hostile, so we only verify the request/reset state machine.
    SignalHandler::Install();
    SignalHandler::Reset();
    HK_CHECK_MSG(!SignalHandler::ShutdownRequested(), "Reset() clears the shutdown request");
    SignalHandler::Reset();
    HK_CHECK(!SignalHandler::ShutdownRequested());
}

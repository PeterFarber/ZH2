#include "Hockey/Core/HeadlessApplication.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/SignalHandler.hpp"
#include "Hockey/Core/Timer.hpp"
namespace Hockey {
namespace {
// Upper bound on ticks simulated in a single iteration. If the host falls
// further behind than this, we drop the surplus to avoid the spiral of death.
constexpr int kMaxTicksPerFrame = 250;
}
void HeadlessApplication::SetTickRate(double tickRate) { m_Timestep.SetTickRate(tickRate); }
void HeadlessApplication::SetSleepWhenIdle(bool sleepWhenIdle) { m_SleepWhenIdle = sleepWhenIdle; }
int HeadlessApplication::Run() {
    SignalHandler::Install();
    if (!OnInit()) {
        OnShutdown();
        return 1;
    }
    Timer timer;
    while (IsRunning() && !SignalHandler::ShutdownRequested()) {
        const double deltaSeconds = timer.ElapsedSeconds();
        timer.Reset();

        int ticks = m_Timestep.Accumulate(deltaSeconds);
        if (ticks > kMaxTicksPerFrame) {
            HK_CORE_WARN("Tick overrun: {} ticks queued in one frame, clamping to {}",
                         ticks, kMaxTicksPerFrame);
            ticks = kMaxTicksPerFrame;
        }
        for (int i = 0; i < ticks; ++i) {
            OnFixedUpdate(m_Timestep.GetFixedDeltaSeconds());
            m_Timestep.AdvanceTick();
        }
        if (m_SleepWhenIdle) {
            Platform::SleepMilliseconds(1);
        } else {
            Platform::YieldThread();
        }
    }
    OnShutdown();
    return 0;
}
}

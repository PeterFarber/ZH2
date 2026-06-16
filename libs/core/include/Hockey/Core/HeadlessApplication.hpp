#pragma once
#include "Hockey/Core/Application.hpp"
#include "Hockey/Core/FixedTimestep.hpp"
namespace Hockey {
class HeadlessApplication : public Application {
public:
    using Application::Application;
    int Run() override;

protected:
    void SetTickRate(double tickRate);
    // When true (default) the loop sleeps ~1ms between iterations to keep CPU
    // usage low. When false it busy-yields, trading CPU for lower latency.
    // Driven by the server's `app.sleep_when_idle` config key.
    void SetSleepWhenIdle(bool sleepWhenIdle);
    virtual void OnFixedUpdate(double fixedDeltaSeconds) = 0;

private:
    FixedTimestep m_Timestep{60.0};
    bool m_SleepWhenIdle = true;
};
} // namespace Hockey

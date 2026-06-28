#include "Hockey/Core/FixedTimestep.hpp"
namespace Hockey {
namespace {
// Small tolerance so a perfectly-sized accumulation (e.g. 1.0s at 60Hz)
// reliably yields the expected whole number of ticks despite float rounding.
constexpr double kTickEpsilon = 1e-9;
}
FixedTimestep::FixedTimestep(double tickRate) { SetTickRate(tickRate); }
void FixedTimestep::SetTickRate(double tickRate) {
    m_TickRate = tickRate > 0.0 ? tickRate : 60.0;
    m_FixedDelta = 1.0 / m_TickRate;
}
double FixedTimestep::GetTickRate() const { return m_TickRate; }
double FixedTimestep::GetFixedDeltaSeconds() const { return m_FixedDelta; }
double FixedTimestep::GetInterpolationAlpha() const {
    if (m_FixedDelta <= 0.0) {
        return 0.0;
    }
    const double alpha = m_Accumulator / m_FixedDelta;
    if (alpha <= 0.0) {
        return 0.0;
    }
    if (alpha >= 1.0) {
        return 1.0;
    }
    return alpha;
}
uint64_t FixedTimestep::GetTick() const { return m_Tick; }
int FixedTimestep::Accumulate(double deltaSeconds) {
    if (deltaSeconds > 0.0) {
        m_Accumulator += deltaSeconds;
    }
    int ticks = 0;
    while (m_Accumulator + kTickEpsilon >= m_FixedDelta) {
        m_Accumulator -= m_FixedDelta;
        ++ticks;
    }
    if (m_Accumulator < 0.0) {
        m_Accumulator = 0.0;
    }
    return ticks;
}
void FixedTimestep::AdvanceTick() { ++m_Tick; }
void FixedTimestep::Reset() {
    m_Accumulator = 0.0;
    m_Tick = 0;
}
}

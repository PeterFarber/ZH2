#pragma once
#include <cstdint>
namespace Hockey {
class FixedTimestep {
public:
    explicit FixedTimestep(double tickRate = 60.0);

    void SetTickRate(double tickRate);
    double GetTickRate() const;

    double GetFixedDeltaSeconds() const;
    uint64_t GetTick() const;

    int Accumulate(double deltaSeconds);
    void AdvanceTick();
    void Reset();

private:
    double m_TickRate = 60.0;
    double m_FixedDelta = 1.0 / 60.0;
    double m_Accumulator = 0.0;
    uint64_t m_Tick = 0;
};
}

#include "Test.hpp"
#include "Hockey/Core/FixedTimestep.hpp"
#include <cmath>
#include <cstdint>
using namespace Hockey;
void RunFixedTickTests() {
    HockeyTest::BeginSuite("FixedTickTests");

    FixedTimestep timestep(60.0);
    HK_CHECK(std::abs(timestep.GetFixedDeltaSeconds() - (1.0 / 60.0)) < 1e-9);
    HK_CHECK_EQ(timestep.GetTickRate(), 60.0);

    const int ticks = timestep.Accumulate(1.0);
    HK_CHECK_EQ(ticks, 60);

    for (int i = 0; i < ticks; ++i) {
        timestep.AdvanceTick();
    }
    HK_CHECK_EQ(timestep.GetTick(), static_cast<uint64_t>(60));

    timestep.Reset();
    HK_CHECK_EQ(timestep.GetTick(), static_cast<uint64_t>(0));
    HK_CHECK_EQ(timestep.Accumulate(0.0), 0);
}

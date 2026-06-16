#include "Test.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/Core/Time.hpp"
using namespace Hockey;
void RunTimeTests() {
    HockeyTest::BeginSuite("TimeTests");

    const double seconds0 = Time::NowSeconds();
    const uint64_t nanos0 = Time::NowNanoseconds();

    Platform::SleepMilliseconds(2);

    const double seconds1 = Time::NowSeconds();
    const uint64_t nanos1 = Time::NowNanoseconds();

    // steady_clock is monotonic non-decreasing.
    HK_CHECK(seconds1 >= seconds0);
    HK_CHECK(nanos1 >= nanos0);

    // After a ~2ms sleep both clocks must have advanced measurably.
    HK_CHECK(nanos1 > nanos0);
    HK_CHECK((seconds1 - seconds0) >= 0.0005);
}

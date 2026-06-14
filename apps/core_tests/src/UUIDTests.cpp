#include "Test.hpp"
#include "Hockey/Core/UUID.hpp"
#include <cstddef>
#include <unordered_map>
using namespace Hockey;
void RunUUIDTests() {
    HockeyTest::BeginSuite("UUIDTests");

    UUID a;
    UUID b;
    HK_CHECK(a.IsValid());
    HK_CHECK(a.Value() != 0);
    HK_CHECK(a != b);

    UUID fixed(1234);
    HK_CHECK_EQ(fixed.Value(), static_cast<uint64_t>(1234));

    std::unordered_map<UUID, int> map;
    map[a] = 1;
    map[b] = 2;
    HK_CHECK_EQ(map.size(), static_cast<size_t>(2));

    HK_CHECK(!a.ToString().empty());
}

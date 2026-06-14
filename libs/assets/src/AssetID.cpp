#include "Hockey/Assets/AssetID.hpp"

#include <cstdio>
#include <random>

namespace Hockey {

namespace {
uint64_t GenerateRandomNonZero() {
    static thread_local std::mt19937_64 engine{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
    return dist(engine);
}
} // namespace

std::string AssetID::ToString() const {
    char buffer[21];
    std::snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(m_Value));
    return std::string(buffer);
}

AssetID AssetID::Generate() { return AssetID(GenerateRandomNonZero()); }

} // namespace Hockey

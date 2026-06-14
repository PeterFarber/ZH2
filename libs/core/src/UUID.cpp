#include "Hockey/Core/UUID.hpp"
#include <cstdio>
#include <random>
namespace Hockey {
namespace {
uint64_t GenerateRandom() {
    static thread_local std::mt19937_64 engine{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
    return dist(engine);
}
}
UUID::UUID() : m_Value(GenerateRandom()) {}
UUID::UUID(uint64_t value) : m_Value(value) {}
uint64_t UUID::Value() const { return m_Value; }
std::string UUID::ToString() const {
    char buffer[17];
    std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(m_Value));
    return std::string(buffer);
}
bool UUID::IsValid() const { return m_Value != 0; }
}

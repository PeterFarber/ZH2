#pragma once
#include <cstdint>
#include <string>
#include <functional>
namespace Hockey { class UUID { public: UUID(); explicit UUID(uint64_t value); uint64_t Value() const; std::string ToString() const; bool IsValid() const; bool operator==(const UUID& other) const = default; private: uint64_t m_Value=0; }; }
namespace std { template<> struct hash<Hockey::UUID> { size_t operator()(const Hockey::UUID& id) const noexcept { return hash<uint64_t>{}(id.Value()); } }; }

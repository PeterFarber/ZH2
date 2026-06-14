#include "Hockey/Assets/AssetHash.hpp"

#include <array>
#include <fstream>

namespace Hockey {
namespace AssetHash {

namespace {
constexpr uint64_t kOffsetBasis = 1469598103934665603ull;
constexpr uint64_t kPrime = 1099511628211ull;
} // namespace

uint64_t HashBytes(const void* data, size_t size) {
    const auto* bytes = static_cast<const unsigned char*>(data);
    uint64_t hash = kOffsetBasis;
    for (size_t i = 0; i < size; ++i) {
        hash ^= static_cast<uint64_t>(bytes[i]);
        hash *= kPrime;
    }
    return hash;
}

uint64_t HashString(std::string_view text) { return HashBytes(text.data(), text.size()); }

uint64_t HashFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return 0;
    }
    uint64_t hash = kOffsetBasis;
    std::array<char, 64 * 1024> buffer{};
    while (stream) {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize read = stream.gcount();
        for (std::streamsize i = 0; i < read; ++i) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(buffer[static_cast<size_t>(i)]));
            hash *= kPrime;
        }
    }
    return hash;
}

} // namespace AssetHash
} // namespace Hockey

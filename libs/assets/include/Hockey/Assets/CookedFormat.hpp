#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetType.hpp"

namespace Hockey {

// Common header prepended to every binary cooked asset. Written/read field by
// field (little-endian) so it is identical on Windows and Linux regardless of
// struct padding.
struct CookedAssetHeader {
    uint32_t magic = 0;
    uint32_t version = 0;
    uint32_t assetType = 0;
    uint64_t assetID = 0;
    uint64_t sourceHash = 0;
};

namespace CookedFormat {
constexpr uint32_t kMagic = 0x41434B48u; // "HKCA"

CookedAssetHeader MakeHeader(AssetType type, AssetID id, uint64_t sourceHash, uint32_t version);
} // namespace CookedFormat

// Appends primitives/blobs to a byte buffer in a fixed little-endian layout.
class BinaryWriter {
public:
    explicit BinaryWriter(std::vector<std::byte>& buffer) : m_Buffer(buffer) {}

    template <typename T> void Write(const T& value) {
        static_assert(std::is_trivially_copyable_v<T>, "BinaryWriter requires POD");
        const auto* bytes = reinterpret_cast<const std::byte*>(&value);
        m_Buffer.insert(m_Buffer.end(), bytes, bytes + sizeof(T));
    }

    void WriteBytes(const void* data, size_t size) {
        const auto* bytes = static_cast<const std::byte*>(data);
        m_Buffer.insert(m_Buffer.end(), bytes, bytes + size);
    }

    void WriteString(const std::string& value) {
        Write<uint32_t>(static_cast<uint32_t>(value.size()));
        WriteBytes(value.data(), value.size());
    }

    void WriteHeader(const CookedAssetHeader& header) {
        Write(header.magic);
        Write(header.version);
        Write(header.assetType);
        Write(header.assetID);
        Write(header.sourceHash);
    }

private:
    std::vector<std::byte>& m_Buffer;
};

// Reads primitives/blobs written by BinaryWriter. All reads are bounds-checked;
// on overrun the reader enters a failed state (Ok() == false).
class BinaryReader {
public:
    BinaryReader(const std::byte* data, size_t size) : m_Data(data), m_Size(size) {}

    bool Ok() const { return m_Ok; }
    size_t Offset() const { return m_Offset; }
    size_t Remaining() const { return m_Size - m_Offset; }

    template <typename T> T Read() {
        static_assert(std::is_trivially_copyable_v<T>, "BinaryReader requires POD");
        T value{};
        if (!Take(&value, sizeof(T))) {
            m_Ok = false;
        }
        return value;
    }

    bool ReadBytes(void* dst, size_t size) {
        if (!Take(dst, size)) {
            m_Ok = false;
            return false;
        }
        return true;
    }

    std::string ReadString() {
        const uint32_t length = Read<uint32_t>();
        if (!m_Ok || length > Remaining()) {
            m_Ok = false;
            return {};
        }
        std::string value(length, '\0');
        if (length > 0 && !Take(value.data(), length)) {
            m_Ok = false;
            return {};
        }
        return value;
    }

    CookedAssetHeader ReadHeader() {
        CookedAssetHeader header;
        header.magic = Read<uint32_t>();
        header.version = Read<uint32_t>();
        header.assetType = Read<uint32_t>();
        header.assetID = Read<uint64_t>();
        header.sourceHash = Read<uint64_t>();
        return header;
    }

private:
    bool Take(void* dst, size_t size) {
        if (m_Offset + size > m_Size) {
            return false;
        }
        std::memcpy(dst, m_Data + m_Offset, size);
        m_Offset += size;
        return true;
    }

    const std::byte* m_Data;
    size_t m_Size;
    size_t m_Offset = 0;
    bool m_Ok = true;
};

} // namespace Hockey

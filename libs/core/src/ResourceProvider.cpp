#include "Hockey/Core/ResourceProvider.hpp"

#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <limits>

namespace Hockey {
namespace {

constexpr std::array<char, 8> kPackageMagic{'H', 'K', 'P', 'K', 'G', '0', '0', '1'};

std::vector<std::byte> CopyBytes(std::span<const std::byte> bytes) {
    return {bytes.begin(), bytes.end()};
}

void AppendU32(std::vector<std::byte>& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
    }
}

void AppendU64(std::vector<std::byte>& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<std::byte>((value >> shift) & 0xffull));
    }
}

bool ReadU32(std::span<const std::byte> bytes, std::size_t& offset, std::uint32_t& out) {
    if (bytes.size() - offset < 4) {
        return false;
    }
    out = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        out |= static_cast<std::uint32_t>(bytes[offset++]) << shift;
    }
    return true;
}

bool ReadU64(std::span<const std::byte> bytes, std::size_t& offset, std::uint64_t& out) {
    if (bytes.size() - offset < 8) {
        return false;
    }
    out = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        out |= static_cast<std::uint64_t>(bytes[offset++]) << shift;
    }
    return true;
}

std::string BytesToString(const std::vector<std::byte>& bytes) {
    std::string text;
    text.resize(bytes.size());
    std::transform(bytes.begin(), bytes.end(), text.begin(),
                   [](std::byte value) { return static_cast<char>(value); });
    return text;
}

} // namespace

Result<std::string> NormalizeResourcePath(std::string_view resourcePath) {
    std::string text(resourcePath);
    std::replace(text.begin(), text.end(), '\\', '/');
    if (text.empty()) {
        return Result<std::string>::Fail("resource path is empty");
    }

    std::filesystem::path path(text);
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return Result<std::string>::Fail("resource path must be relative: " + text);
    }

    std::filesystem::path normalized;
    for (const std::filesystem::path& part : path.lexically_normal()) {
        const std::string segment = part.generic_string();
        if (segment.empty() || segment == ".") {
            continue;
        }
        if (segment == "..") {
            return Result<std::string>::Fail("resource path cannot contain '..': " + text);
        }
        normalized /= segment;
    }

    const std::string generic = normalized.generic_string();
    if (generic.empty() || generic == ".") {
        return Result<std::string>::Fail("resource path is empty");
    }
    return Result<std::string>::Ok(generic);
}

FileResourceProvider::FileResourceProvider(std::filesystem::path root)
    : m_Root(std::move(root)) {}

std::filesystem::path FileResourceProvider::Resolve(std::string_view resourcePath) const {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    if (!normalized) {
        return {};
    }
    return m_Root / normalized.value;
}

Result<std::string> FileResourceProvider::ReadText(std::string_view resourcePath) const {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    if (!normalized) {
        return Result<std::string>::Fail(normalized.error);
    }
    return FileSystem::ReadTextFile(m_Root / normalized.value);
}

Result<std::vector<std::byte>> FileResourceProvider::ReadBinary(std::string_view resourcePath) const {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    if (!normalized) {
        return Result<std::vector<std::byte>>::Fail(normalized.error);
    }

    const std::filesystem::path path = m_Root / normalized.value;
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<std::vector<std::byte>>::Fail("cannot open resource for read: " + path.string());
    }
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        return Result<std::vector<std::byte>>::Fail("cannot determine resource size: " + path.string());
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream.good() && !stream.eof()) {
        return Result<std::vector<std::byte>>::Fail("failed while reading resource: " + path.string());
    }
    return Result<std::vector<std::byte>>::Ok(std::move(bytes));
}

EmbeddedResourceProvider::EmbeddedResourceProvider(std::vector<ResourcePackageEntry> entries) {
    for (ResourcePackageEntry& entry : entries) {
        AddResource(entry.path, entry.bytes);
    }
}

Result<EmbeddedResourceProvider> EmbeddedResourceProvider::FromPackage(std::span<const std::byte> packageBytes) {
    Result<std::vector<ResourcePackageEntry>> entries = DecodeResourcePackage(packageBytes);
    if (!entries) {
        return Result<EmbeddedResourceProvider>::Fail(entries.error);
    }
    return Result<EmbeddedResourceProvider>::Ok(EmbeddedResourceProvider(std::move(entries.value)));
}

Status EmbeddedResourceProvider::AddResource(std::string_view resourcePath, std::span<const std::byte> bytes) {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    if (!normalized) {
        return Status::Fail(normalized.error);
    }
    m_Resources[normalized.value] = CopyBytes(bytes);
    return Status::Ok();
}

bool EmbeddedResourceProvider::Contains(std::string_view resourcePath) const {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    return normalized && m_Resources.find(normalized.value) != m_Resources.end();
}

std::vector<std::string> EmbeddedResourceProvider::ResourcePaths() const {
    std::vector<std::string> paths;
    paths.reserve(m_Resources.size());
    for (const auto& [path, bytes] : m_Resources) {
        (void)bytes;
        paths.push_back(path);
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

Result<std::string> EmbeddedResourceProvider::ReadText(std::string_view resourcePath) const {
    Result<std::vector<std::byte>> bytes = ReadBinary(resourcePath);
    if (!bytes) {
        return Result<std::string>::Fail(bytes.error);
    }
    return Result<std::string>::Ok(BytesToString(bytes.value));
}

Result<std::vector<std::byte>> EmbeddedResourceProvider::ReadBinary(std::string_view resourcePath) const {
    const Result<std::string> normalized = NormalizeResourcePath(resourcePath);
    if (!normalized) {
        return Result<std::vector<std::byte>>::Fail(normalized.error);
    }
    const auto found = m_Resources.find(normalized.value);
    if (found == m_Resources.end()) {
        return Result<std::vector<std::byte>>::Fail("embedded resource not found: " + normalized.value);
    }
    return Result<std::vector<std::byte>>::Ok(found->second);
}

Result<std::vector<std::byte>> EncodeResourcePackage(std::vector<ResourcePackageEntry> entries) {
    for (ResourcePackageEntry& entry : entries) {
        const Result<std::string> normalized = NormalizeResourcePath(entry.path);
        if (!normalized) {
            return Result<std::vector<std::byte>>::Fail(normalized.error);
        }
        entry.path = normalized.value;
    }
    std::sort(entries.begin(), entries.end(),
              [](const ResourcePackageEntry& a, const ResourcePackageEntry& b) { return a.path < b.path; });
    for (std::size_t i = 1; i < entries.size(); ++i) {
        if (entries[i - 1].path == entries[i].path) {
            return Result<std::vector<std::byte>>::Fail("duplicate resource path: " + entries[i].path);
        }
    }
    if (entries.size() > std::numeric_limits<std::uint32_t>::max()) {
        return Result<std::vector<std::byte>>::Fail("too many resources in package");
    }

    std::vector<std::byte> out;
    out.reserve(kPackageMagic.size() + 4);
    for (const char ch : kPackageMagic) {
        out.push_back(static_cast<std::byte>(ch));
    }
    AppendU32(out, static_cast<std::uint32_t>(entries.size()));
    for (const ResourcePackageEntry& entry : entries) {
        if (entry.path.size() > std::numeric_limits<std::uint32_t>::max()) {
            return Result<std::vector<std::byte>>::Fail("resource path too long: " + entry.path);
        }
        AppendU32(out, static_cast<std::uint32_t>(entry.path.size()));
        AppendU64(out, static_cast<std::uint64_t>(entry.bytes.size()));
        out.insert(out.end(), reinterpret_cast<const std::byte*>(entry.path.data()),
                   reinterpret_cast<const std::byte*>(entry.path.data() + entry.path.size()));
        out.insert(out.end(), entry.bytes.begin(), entry.bytes.end());
    }
    return Result<std::vector<std::byte>>::Ok(std::move(out));
}

Result<std::vector<ResourcePackageEntry>> DecodeResourcePackage(std::span<const std::byte> packageBytes) {
    if (packageBytes.size() < kPackageMagic.size() + 4) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("resource package is truncated");
    }
    for (std::size_t i = 0; i < kPackageMagic.size(); ++i) {
        if (packageBytes[i] != static_cast<std::byte>(kPackageMagic[i])) {
            return Result<std::vector<ResourcePackageEntry>>::Fail("resource package has invalid magic");
        }
    }

    std::size_t offset = kPackageMagic.size();
    std::uint32_t count = 0;
    if (!ReadU32(packageBytes, offset, count)) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("resource package is missing entry count");
    }

    std::vector<ResourcePackageEntry> entries;
    entries.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        std::uint32_t pathSize = 0;
        std::uint64_t dataSize = 0;
        if (!ReadU32(packageBytes, offset, pathSize) || !ReadU64(packageBytes, offset, dataSize)) {
            return Result<std::vector<ResourcePackageEntry>>::Fail("resource package entry header is truncated");
        }
        if (packageBytes.size() - offset < pathSize) {
            return Result<std::vector<ResourcePackageEntry>>::Fail("resource package path is truncated");
        }
        std::string path;
        path.resize(pathSize);
        for (std::uint32_t j = 0; j < pathSize; ++j) {
            path[j] = static_cast<char>(packageBytes[offset++]);
        }
        if (dataSize > packageBytes.size() - offset) {
            return Result<std::vector<ResourcePackageEntry>>::Fail("resource package payload is truncated");
        }
        ResourcePackageEntry entry;
        const Result<std::string> normalized = NormalizeResourcePath(path);
        if (!normalized) {
            return Result<std::vector<ResourcePackageEntry>>::Fail(normalized.error);
        }
        entry.path = normalized.value;
        entry.bytes.assign(packageBytes.begin() + static_cast<std::ptrdiff_t>(offset),
                           packageBytes.begin() + static_cast<std::ptrdiff_t>(offset + dataSize));
        offset += static_cast<std::size_t>(dataSize);
        entries.push_back(std::move(entry));
    }
    if (offset != packageBytes.size()) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("resource package has trailing bytes");
    }
    std::sort(entries.begin(), entries.end(),
              [](const ResourcePackageEntry& a, const ResourcePackageEntry& b) { return a.path < b.path; });
    for (std::size_t i = 1; i < entries.size(); ++i) {
        if (entries[i - 1].path == entries[i].path) {
            return Result<std::vector<ResourcePackageEntry>>::Fail("duplicate resource path: " + entries[i].path);
        }
    }
    return Result<std::vector<ResourcePackageEntry>>::Ok(std::move(entries));
}

Status WriteResourcePackageFile(const std::filesystem::path& path, std::vector<ResourcePackageEntry> entries) {
    Result<std::vector<std::byte>> encoded = EncodeResourcePackage(std::move(entries));
    if (!encoded) {
        return Status::Fail(encoded.error);
    }
    if (path.has_parent_path()) {
        if (const Status dirs = FileSystem::CreateDirectories(path.parent_path()); !dirs) {
            return dirs;
        }
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return Status::Fail("cannot open package for write: " + path.string());
    }
    if (!encoded.value.empty()) {
        stream.write(reinterpret_cast<const char*>(encoded.value.data()), static_cast<std::streamsize>(encoded.value.size()));
    }
    if (!stream.good()) {
        return Status::Fail("failed while writing package: " + path.string());
    }
    return Status::Ok();
}

Result<std::vector<ResourcePackageEntry>> ReadResourcePackageFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("cannot open package for read: " + path.string());
    }
    const std::streamoff size = stream.tellg();
    if (size < 0) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("cannot determine package size: " + path.string());
    }
    stream.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream.good() && !stream.eof()) {
        return Result<std::vector<ResourcePackageEntry>>::Fail("failed while reading package: " + path.string());
    }
    return DecodeResourcePackage(bytes);
}

} // namespace Hockey

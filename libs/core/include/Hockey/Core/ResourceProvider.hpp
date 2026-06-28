#pragma once

#include "Hockey/Core/Result.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hockey {

struct ResourcePackageEntry {
    std::string path;
    std::vector<std::byte> bytes;
};

class ResourceProvider {
public:
    virtual ~ResourceProvider() = default;

    virtual Result<std::string> ReadText(std::string_view resourcePath) const = 0;
    virtual Result<std::vector<std::byte>> ReadBinary(std::string_view resourcePath) const = 0;
};

Result<std::string> NormalizeResourcePath(std::string_view resourcePath);

class FileResourceProvider final : public ResourceProvider {
public:
    explicit FileResourceProvider(std::filesystem::path root);

    Result<std::string> ReadText(std::string_view resourcePath) const override;
    Result<std::vector<std::byte>> ReadBinary(std::string_view resourcePath) const override;

    const std::filesystem::path& Root() const {
        return m_Root;
    }

private:
    std::filesystem::path Resolve(std::string_view resourcePath) const;

    std::filesystem::path m_Root;
};

class EmbeddedResourceProvider final : public ResourceProvider {
public:
    EmbeddedResourceProvider() = default;
    explicit EmbeddedResourceProvider(std::vector<ResourcePackageEntry> entries);

    static Result<EmbeddedResourceProvider> FromPackage(std::span<const std::byte> packageBytes);

    Status AddResource(std::string_view resourcePath, std::span<const std::byte> bytes);
    bool Contains(std::string_view resourcePath) const;
    std::vector<std::string> ResourcePaths() const;

    Result<std::string> ReadText(std::string_view resourcePath) const override;
    Result<std::vector<std::byte>> ReadBinary(std::string_view resourcePath) const override;

private:
    std::unordered_map<std::string, std::vector<std::byte>> m_Resources;
};

Result<std::vector<std::byte>> EncodeResourcePackage(std::vector<ResourcePackageEntry> entries);
Result<std::vector<ResourcePackageEntry>> DecodeResourcePackage(std::span<const std::byte> packageBytes);
Status WriteResourcePackageFile(const std::filesystem::path& path, std::vector<ResourcePackageEntry> entries);
Result<std::vector<ResourcePackageEntry>> ReadResourcePackageFile(const std::filesystem::path& path);

} // namespace Hockey

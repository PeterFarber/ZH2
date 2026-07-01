#include "Hockey/Assets/Cookers/AudioCooker.hpp"

#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Assets/Runtime/AudioLoader.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <fstream>
#include <iterator>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

Result<std::vector<uint8_t>> ReadAllBytes(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return Result<std::vector<uint8_t>>::Fail("cannot open audio source: " + path.string());
    }
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if (!stream.good() && !stream.eof()) {
        return Result<std::vector<uint8_t>>::Fail("failed while reading audio source: " + path.string());
    }
    return Result<std::vector<uint8_t>>::Ok(std::move(bytes));
}

Status WriteAllBytes(const fs::path& path, const std::vector<std::byte>& bytes) {
    if (path.has_parent_path()) {
        const Status dirs = FileSystem::CreateDirectories(path.parent_path());
        if (!dirs) {
            return dirs;
        }
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return Status::Fail("cannot open cooked audio for write: " + path.string());
    }
    if (!bytes.empty()) {
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    if (!stream.good()) {
        return Status::Fail("failed while writing cooked audio: " + path.string());
    }
    return Status::Ok();
}

} // namespace

CookResult AudioCooker::Cook(const CookContext& context) {
    CookResult result;
    const AssetMetadata& metadata = context.metadata;
    const fs::path rawAbsolute = context.projectRoot / metadata.rawPath;

    Result<std::vector<uint8_t>> bytes = ReadAllBytes(rawAbsolute);
    if (!bytes) {
        result.success = false;
        result.error = bytes.error;
        return result;
    }

    AudioAsset asset;
    asset.id = metadata.id;
    asset.debugName = metadata.name.empty() ? metadata.rawPath.filename().string() : metadata.name;
    asset.sourceExtension = metadata.rawPath.extension().string();
    asset.encodedBytes = std::move(bytes.value);

    const fs::path cookedAbsolute = context.cookedRoot / "assets" / AssetPath::CookedSubdirectory(AssetType::Audio) /
                                    (metadata.id.ToString() + AssetPath::CookedExtension(AssetType::Audio));
    const Status wrote = WriteAllBytes(cookedAbsolute, AudioLoader::EncodeCooked(asset, metadata.rawFileHash));
    if (!wrote) {
        result.success = false;
        result.error = wrote.error;
        return result;
    }

    result.success = true;
    result.cookedPath = cookedAbsolute;
    return result;
}

} // namespace Hockey

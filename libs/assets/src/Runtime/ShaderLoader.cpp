#include "Hockey/Assets/Runtime/ShaderLoader.hpp"

#include "Hockey/Core/FileSystem.hpp"

#include <fstream>

namespace Hockey {
namespace fs = std::filesystem;

Status ShaderLoader::WriteSpv(const fs::path& cookedPath, const std::vector<uint32_t>& spirv) {
    if (cookedPath.has_parent_path()) {
        const Status dirs = FileSystem::CreateDirectories(cookedPath.parent_path());
        if (!dirs) {
            return dirs;
        }
    }
    std::ofstream out(cookedPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return Status::Fail("cannot write cooked shader: " + cookedPath.string());
    }
    out.write(reinterpret_cast<const char*>(spirv.data()),
              static_cast<std::streamsize>(spirv.size() * sizeof(uint32_t)));
    return Status::Ok();
}

Result<ShaderAsset> ShaderLoader::LoadCooked(const fs::path& cookedPath, ShaderAssetStage stage) {
    std::ifstream stream(cookedPath, std::ios::binary | std::ios::ate);
    if (!stream.is_open()) {
        return Result<ShaderAsset>::Fail("cannot open cooked shader: " + cookedPath.string());
    }
    const std::streamsize size = stream.tellg();
    if (size <= 0 || (size % 4) != 0) {
        return Result<ShaderAsset>::Fail("cooked shader is not valid SPIR-V: " + cookedPath.string());
    }
    stream.seekg(0, std::ios::beg);

    ShaderAsset asset;
    asset.stage = stage;
    asset.spirv.resize(static_cast<size_t>(size) / sizeof(uint32_t));
    stream.read(reinterpret_cast<char*>(asset.spirv.data()), size);
    if (!stream) {
        return Result<ShaderAsset>::Fail("failed to read cooked shader: " + cookedPath.string());
    }
    return Result<ShaderAsset>::Ok(std::move(asset));
}

Result<ShaderAsset> ShaderLoader::LoadCooked(const fs::path& cookedPath) {
    return LoadCooked(cookedPath, ShaderAssetStage::Unknown);
}

} // namespace Hockey

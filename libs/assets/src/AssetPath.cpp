#include "Hockey/Assets/AssetPath.hpp"

namespace Hockey {
namespace AssetPath {

namespace fs = std::filesystem;

fs::path Normalize(const fs::path& path) {
    if (path.empty()) {
        return path;
    }
    return path.lexically_normal().generic_string();
}

fs::path ToProjectRelative(const fs::path& root, const fs::path& path) {
    std::error_code ec;
    const fs::path normalizedRoot = fs::weakly_canonical(root, ec);
    const fs::path base = ec ? root.lexically_normal() : normalizedRoot;

    fs::path absolute = path;
    if (!absolute.is_absolute()) {
        absolute = (base / path);
    }
    absolute = absolute.lexically_normal();

    const fs::path relative = absolute.lexically_relative(base.lexically_normal());
    if (relative.empty() || relative.native()[0] == '.') {
        // Either not relative or escapes the root (starts with ".."); fall back
        // to the normalized input so we never produce a broken reference.
        if (!relative.empty() && relative.string().rfind("..", 0) != 0) {
            return relative.generic_string();
        }
        return absolute.generic_string();
    }
    return relative.generic_string();
}

fs::path ToAbsolute(const fs::path& root, const fs::path& path) {
    if (path.is_absolute()) {
        return path.lexically_normal();
    }
    return (root / path).lexically_normal();
}

bool IsUnderRoot(const fs::path& root, const fs::path& path) {
    const fs::path normalizedRoot = root.lexically_normal();
    fs::path absolute = path.is_absolute() ? path : (normalizedRoot / path);
    absolute = absolute.lexically_normal();
    const fs::path relative = absolute.lexically_relative(normalizedRoot);
    return !relative.empty() && relative.string().rfind("..", 0) != 0 && relative != ".";
}

fs::path MetadataSidecar(const fs::path& rawPath) {
    return fs::path(rawPath.generic_string() + ".meta.yaml");
}

bool IsMetadataSidecar(const fs::path& path) {
    const std::string s = path.generic_string();
    static constexpr const char* kSuffix = ".meta.yaml";
    const std::string suffix = kSuffix;
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string CookedSubdirectory(AssetType type) {
    switch (type) {
    case AssetType::Texture:
        return "textures";
    case AssetType::Mesh:
        return "meshes";
    case AssetType::Model:
        return "models";
    case AssetType::Material:
        return "materials";
    case AssetType::Shader:
        return "shaders";
    case AssetType::Scene:
        return "scenes";
    case AssetType::Prefab:
        return "prefabs";
    case AssetType::Audio:
        return "audio";
    case AssetType::Skeleton:
        return "skeletons";
    case AssetType::Animation:
        return "animations";
    case AssetType::Unknown:
        break;
    }
    return "misc";
}

std::string CookedExtension(AssetType type) {
    switch (type) {
    case AssetType::Texture:
        return ".tex.bin";
    case AssetType::Mesh:
        return ".mesh.bin";
    case AssetType::Model:
        return ".model.bin";
    case AssetType::Material:
        return ".material.yaml";
    case AssetType::Shader:
        return ".spv";
    case AssetType::Scene:
        return ".scene.yaml";
    case AssetType::Prefab:
        return ".prefab.yaml";
    case AssetType::Audio:
        return ".audio.bin";
    case AssetType::Skeleton:
        return ".skel.bin";
    case AssetType::Animation:
        return ".anim.bin";
    case AssetType::Unknown:
        break;
    }
    return ".bin";
}

fs::path CookedPath(const fs::path& cookedRoot, AssetType type, AssetID id) {
    const fs::path relative =
        cookedRoot / "assets" / CookedSubdirectory(type) / (id.ToString() + CookedExtension(type));
    return relative.generic_string();
}

} // namespace AssetPath
} // namespace Hockey

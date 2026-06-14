#include "Hockey/Editor/Project/FileTypeRegistry.hpp"

#include <algorithm>
#include <string>

namespace Hockey {

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

bool EndsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

} // namespace

FileTypeInfo FileTypeRegistry::Classify(const std::filesystem::path& path) {
    const std::string name = ToLower(path.filename().string());

    if (EndsWith(name, ".scene.yaml")) {
        return {EditorFileType::Scene, "Scene", true};
    }
    if (EndsWith(name, ".prefab.yaml")) {
        return {EditorFileType::Prefab, "Prefab", true};
    }
    if (EndsWith(name, ".material.yaml")) {
        return {EditorFileType::Material, "Material", true};
    }

    const std::string ext = ToLower(path.extension().string());
    if (ext == ".toml") {
        return {EditorFileType::Toml, "Config", true};
    }
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".geom") {
        return {EditorFileType::ShaderSource, "Shader", true};
    }
    if (ext == ".spv") {
        return {EditorFileType::ShaderBinary, "Shader Binary", false};
    }
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".ktx" ||
        ext == ".ktx2" || ext == ".hdr") {
        return {EditorFileType::Image, "Texture", true};
    }
    if (ext == ".gltf" || ext == ".glb") {
        return {EditorFileType::Model, "Model", true};
    }
    if (ext == ".obj" || ext == ".fbx" || ext == ".dds") {
        return {EditorFileType::Model, "Model", false};
    }
    if (ext == ".yaml" || ext == ".yml" || ext == ".json" || ext == ".txt" || ext == ".md") {
        return {EditorFileType::Text, "Text", true};
    }
    return {EditorFileType::Unknown, "File", false};
}

bool FileTypeRegistry::IsScene(const std::filesystem::path& path) {
    return Classify(path).type == EditorFileType::Scene;
}

bool FileTypeRegistry::IsPrefab(const std::filesystem::path& path) {
    return Classify(path).type == EditorFileType::Prefab;
}

} // namespace Hockey

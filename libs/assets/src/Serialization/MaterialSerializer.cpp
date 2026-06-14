#include "Hockey/Assets/Serialization/MaterialSerializer.hpp"

#include "Hockey/Core/FileSystem.hpp"

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
void EmitTexture(YAML::Emitter& out, const char* key, const std::string& path) {
    if (!path.empty()) {
        out << YAML::Key << key << YAML::Value << path;
    }
}
} // namespace

std::string MaterialSerializer::Serialize(const MaterialSource& source) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << source.name;
    out << YAML::Key << "Type" << YAML::Value << "PBR";
    out << YAML::Key << "BaseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << source.baseColor.r
        << source.baseColor.g << source.baseColor.b << source.baseColor.a << YAML::EndSeq;
    out << YAML::Key << "Metallic" << YAML::Value << source.metallic;
    out << YAML::Key << "Roughness" << YAML::Value << source.roughness;
    out << YAML::Key << "NormalStrength" << YAML::Value << source.normalStrength;
    out << YAML::Key << "OcclusionStrength" << YAML::Value << source.occlusionStrength;
    out << YAML::Key << "EmissiveColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << source.emissiveColor.r
        << source.emissiveColor.g << source.emissiveColor.b << YAML::EndSeq;
    out << YAML::Key << "EmissiveStrength" << YAML::Value << source.emissiveStrength;
    out << YAML::Key << "AlphaMode" << YAML::Value << source.alphaMode;
    out << YAML::Key << "AlphaCutoff" << YAML::Value << source.alphaCutoff;

    out << YAML::Key << "Textures" << YAML::Value << YAML::BeginMap;
    EmitTexture(out, "BaseColor", source.baseColorTexture);
    EmitTexture(out, "Normal", source.normalTexture);
    EmitTexture(out, "MetallicRoughness", source.metallicRoughnessTexture);
    EmitTexture(out, "Occlusion", source.occlusionTexture);
    EmitTexture(out, "Emissive", source.emissiveTexture);
    out << YAML::EndMap;

    out << YAML::EndMap;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

Result<MaterialSource> MaterialSerializer::Deserialize(const std::string& text) {
    YAML::Node root;
    try {
        root = YAML::Load(text);
    } catch (const YAML::Exception& e) {
        return Result<MaterialSource>::Fail(std::string("material parse error: ") + e.what());
    }

    const YAML::Node node = root["Material"];
    if (!node || !node.IsMap()) {
        return Result<MaterialSource>::Fail("material missing top-level 'Material' map");
    }

    MaterialSource source;
    if (node["Name"])
        source.name = node["Name"].as<std::string>();
    if (node["BaseColor"] && node["BaseColor"].size() == 4) {
        source.baseColor = {node["BaseColor"][0].as<float>(), node["BaseColor"][1].as<float>(),
                            node["BaseColor"][2].as<float>(), node["BaseColor"][3].as<float>()};
    }
    if (node["Metallic"])
        source.metallic = node["Metallic"].as<float>();
    if (node["Roughness"])
        source.roughness = node["Roughness"].as<float>();
    if (node["NormalStrength"])
        source.normalStrength = node["NormalStrength"].as<float>();
    if (node["OcclusionStrength"])
        source.occlusionStrength = node["OcclusionStrength"].as<float>();
    if (node["EmissiveColor"] && node["EmissiveColor"].size() == 3) {
        source.emissiveColor = {node["EmissiveColor"][0].as<float>(), node["EmissiveColor"][1].as<float>(),
                                node["EmissiveColor"][2].as<float>()};
    }
    if (node["EmissiveStrength"])
        source.emissiveStrength = node["EmissiveStrength"].as<float>();
    if (node["AlphaMode"])
        source.alphaMode = node["AlphaMode"].as<std::string>();
    if (node["AlphaCutoff"])
        source.alphaCutoff = node["AlphaCutoff"].as<float>();

    if (const YAML::Node textures = node["Textures"]) {
        auto read = [&](const char* key) -> std::string {
            return textures[key] ? textures[key].as<std::string>() : std::string{};
        };
        source.baseColorTexture = read("BaseColor");
        source.normalTexture = read("Normal");
        source.metallicRoughnessTexture = read("MetallicRoughness");
        source.occlusionTexture = read("Occlusion");
        source.emissiveTexture = read("Emissive");
    }
    return Result<MaterialSource>::Ok(std::move(source));
}

Result<MaterialSource> MaterialSerializer::LoadFile(const fs::path& path) {
    const Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        return Result<MaterialSource>::Fail(text.error);
    }
    return Deserialize(text.value);
}

Status MaterialSerializer::SaveFile(const fs::path& path, const MaterialSource& source) {
    return FileSystem::WriteTextFile(path, Serialize(source));
}

std::vector<std::string> MaterialSerializer::TexturePaths(const MaterialSource& source) {
    std::vector<std::string> paths;
    for (const std::string* slot : {&source.baseColorTexture, &source.normalTexture, &source.metallicRoughnessTexture,
                                    &source.occlusionTexture, &source.emissiveTexture}) {
        if (!slot->empty()) {
            paths.push_back(*slot);
        }
    }
    return paths;
}

} // namespace Hockey

#include "Hockey/Assets/Runtime/MaterialLoader.hpp"

#include "Hockey/Core/FileSystem.hpp"

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace fs = std::filesystem;

namespace {
void EmitId(YAML::Emitter& out, const char* key, AssetID id) {
    out << YAML::Key << key << YAML::Value << id.Value();
}
AssetID ReadId(const YAML::Node& node) {
    return node ? AssetID(node.as<std::uint64_t>()) : AssetID();
}
} // namespace

std::string MaterialLoader::EncodeCooked(const MaterialAsset& material) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "CookedMaterial" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ID" << YAML::Value << material.id.Value();
    out << YAML::Key << "Name" << YAML::Value << material.name;
    out << YAML::Key << "BaseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << material.baseColor.r << material.baseColor.g << material.baseColor.b
        << material.baseColor.a << YAML::EndSeq;
    out << YAML::Key << "Metallic" << YAML::Value << material.metallic;
    out << YAML::Key << "Roughness" << YAML::Value << material.roughness;
    out << YAML::Key << "NormalStrength" << YAML::Value << material.normalStrength;
    out << YAML::Key << "OcclusionStrength" << YAML::Value << material.occlusionStrength;
    out << YAML::Key << "EmissiveColor" << YAML::Value << YAML::Flow << YAML::BeginSeq
        << material.emissiveColor.r << material.emissiveColor.g << material.emissiveColor.b
        << YAML::EndSeq;
    out << YAML::Key << "EmissiveStrength" << YAML::Value << material.emissiveStrength;
    out << YAML::Key << "AlphaMode" << YAML::Value << material.alphaMode;
    out << YAML::Key << "AlphaCutoff" << YAML::Value << material.alphaCutoff;
    out << YAML::Key << "Tiling" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.tiling.x << material.tiling.y
        << YAML::EndSeq;
    out << YAML::Key << "Offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << material.offset.x << material.offset.y
        << YAML::EndSeq;
    out << YAML::Key << "Textures" << YAML::Value << YAML::BeginMap;
    EmitId(out, "BaseColor", material.baseColorTexture);
    EmitId(out, "Normal", material.normalTexture);
    EmitId(out, "MetallicRoughness", material.metallicRoughnessTexture);
    EmitId(out, "Occlusion", material.occlusionTexture);
    EmitId(out, "Emissive", material.emissiveTexture);
    out << YAML::EndMap;
    out << YAML::EndMap;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

Result<MaterialAsset> MaterialLoader::DecodeCooked(const std::string& text) {
    YAML::Node root;
    try {
        root = YAML::Load(text);
    } catch (const YAML::Exception& e) {
        return Result<MaterialAsset>::Fail(std::string("cooked material parse error: ") + e.what());
    }
    const YAML::Node node = root["CookedMaterial"];
    if (!node || !node.IsMap()) {
        return Result<MaterialAsset>::Fail("cooked material missing 'CookedMaterial'");
    }

    MaterialAsset material;
    if (node["ID"])
        material.id = AssetID(node["ID"].as<std::uint64_t>());
    if (node["Name"])
        material.name = node["Name"].as<std::string>();
    if (node["BaseColor"] && node["BaseColor"].size() == 4) {
        material.baseColor = {node["BaseColor"][0].as<float>(), node["BaseColor"][1].as<float>(),
                              node["BaseColor"][2].as<float>(), node["BaseColor"][3].as<float>()};
    }
    if (node["Metallic"])
        material.metallic = node["Metallic"].as<float>();
    if (node["Roughness"])
        material.roughness = node["Roughness"].as<float>();
    if (node["NormalStrength"])
        material.normalStrength = node["NormalStrength"].as<float>();
    if (node["OcclusionStrength"])
        material.occlusionStrength = node["OcclusionStrength"].as<float>();
    if (node["EmissiveColor"] && node["EmissiveColor"].size() == 3) {
        material.emissiveColor = {node["EmissiveColor"][0].as<float>(),
                                  node["EmissiveColor"][1].as<float>(),
                                  node["EmissiveColor"][2].as<float>()};
    }
    if (node["EmissiveStrength"])
        material.emissiveStrength = node["EmissiveStrength"].as<float>();
    if (node["AlphaMode"])
        material.alphaMode = node["AlphaMode"].as<std::string>();
    if (node["AlphaCutoff"])
        material.alphaCutoff = node["AlphaCutoff"].as<float>();
    if (node["Tiling"] && node["Tiling"].size() == 2) {
        material.tiling = {node["Tiling"][0].as<float>(), node["Tiling"][1].as<float>()};
    }
    if (node["Offset"] && node["Offset"].size() == 2) {
        material.offset = {node["Offset"][0].as<float>(), node["Offset"][1].as<float>()};
    }

    if (const YAML::Node textures = node["Textures"]) {
        material.baseColorTexture = ReadId(textures["BaseColor"]);
        material.normalTexture = ReadId(textures["Normal"]);
        material.metallicRoughnessTexture = ReadId(textures["MetallicRoughness"]);
        material.occlusionTexture = ReadId(textures["Occlusion"]);
        material.emissiveTexture = ReadId(textures["Emissive"]);
    }
    return Result<MaterialAsset>::Ok(std::move(material));
}

Result<MaterialAsset> MaterialLoader::LoadCooked(const fs::path& cookedPath) {
    const Result<std::string> text = FileSystem::ReadTextFile(cookedPath);
    if (!text) {
        return Result<MaterialAsset>::Fail(text.error);
    }
    return DecodeCooked(text.value);
}

} // namespace Hockey

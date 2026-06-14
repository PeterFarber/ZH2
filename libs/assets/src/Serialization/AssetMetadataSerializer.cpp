#include "Hockey/Assets/Serialization/AssetMetadataSerializer.hpp"

#include "Hockey/Core/FileSystem.hpp"
#include "Serialization/MetadataYaml.hpp"

namespace Hockey {

std::string AssetMetadataSerializer::Serialize(const AssetMetadata& metadata) {
    YAML::Emitter out;
    MetadataYaml::Emit(out, metadata);
    return std::string(out.c_str());
}

bool AssetMetadataSerializer::Deserialize(const std::string& text, AssetMetadata& out) {
    YAML::Node root;
    try {
        root = YAML::Load(text);
    } catch (const YAML::Exception&) {
        return false;
    }
    return MetadataYaml::Read(root, out);
}

Status AssetMetadataSerializer::SaveSidecar(const std::filesystem::path& sidecarPath,
                                            const AssetMetadata& metadata) {
    return FileSystem::WriteTextFile(sidecarPath, Serialize(metadata));
}

Result<AssetMetadata> AssetMetadataSerializer::LoadSidecar(const std::filesystem::path& sidecarPath) {
    const Result<std::string> text = FileSystem::ReadTextFile(sidecarPath);
    if (!text) {
        return Result<AssetMetadata>::Fail(text.error);
    }
    AssetMetadata metadata;
    if (!Deserialize(text.value, metadata)) {
        return Result<AssetMetadata>::Fail("failed to parse metadata sidecar: " +
                                           sidecarPath.string());
    }
    metadata.metadataPath = sidecarPath;
    return Result<AssetMetadata>::Ok(std::move(metadata));
}

} // namespace Hockey

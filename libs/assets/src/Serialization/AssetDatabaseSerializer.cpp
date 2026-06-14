#include "Hockey/Assets/Serialization/AssetDatabaseSerializer.hpp"

#include "Hockey/Core/FileSystem.hpp"
#include "Serialization/MetadataYaml.hpp"

namespace Hockey {

namespace {
constexpr int kDatabaseVersion = 1;
}

std::string AssetDatabaseSerializer::Serialize(const std::vector<const AssetMetadata*>& assets) {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "AssetDatabase" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Version" << YAML::Value << kDatabaseVersion;
    out << YAML::Key << "Count" << YAML::Value << static_cast<int>(assets.size());
    out << YAML::EndMap;

    out << YAML::Key << "Assets" << YAML::Value << YAML::BeginSeq;
    for (const AssetMetadata* metadata : assets) {
        if (metadata != nullptr) {
            MetadataYaml::Emit(out, *metadata);
        }
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

bool AssetDatabaseSerializer::Deserialize(const std::string& text, std::vector<AssetMetadata>& out) {
    YAML::Node root;
    try {
        root = YAML::Load(text);
    } catch (const YAML::Exception&) {
        return false;
    }
    out.clear();
    const YAML::Node assets = root["Assets"];
    if (!assets || !assets.IsSequence()) {
        // An empty/sparse database file with no Assets sequence is still valid.
        return true;
    }
    for (const auto& node : assets) {
        AssetMetadata metadata;
        if (MetadataYaml::Read(node, metadata)) {
            out.push_back(std::move(metadata));
        }
    }
    return true;
}

Status AssetDatabaseSerializer::Save(const std::filesystem::path& path,
                                     const std::vector<const AssetMetadata*>& assets) {
    return FileSystem::WriteTextFile(path, Serialize(assets));
}

Result<std::vector<AssetMetadata>> AssetDatabaseSerializer::Load(const std::filesystem::path& path) {
    const Result<std::string> text = FileSystem::ReadTextFile(path);
    if (!text) {
        return Result<std::vector<AssetMetadata>>::Fail(text.error);
    }
    std::vector<AssetMetadata> assets;
    if (!Deserialize(text.value, assets)) {
        return Result<std::vector<AssetMetadata>>::Fail("failed to parse asset database: " +
                                                        path.string());
    }
    return Result<std::vector<AssetMetadata>>::Ok(std::move(assets));
}

} // namespace Hockey

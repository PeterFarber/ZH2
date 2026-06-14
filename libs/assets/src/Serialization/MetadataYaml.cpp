#include "Serialization/MetadataYaml.hpp"

namespace Hockey {
namespace MetadataYaml {

namespace {

void EmitIdList(YAML::Emitter& out, const char* key, const std::vector<AssetID>& ids) {
    out << YAML::Key << key << YAML::Value << YAML::Flow << YAML::BeginSeq;
    for (const AssetID& id : ids) {
        out << id.Value();
    }
    out << YAML::EndSeq;
}

std::vector<AssetID> ReadIdList(const YAML::Node& node) {
    std::vector<AssetID> ids;
    if (node && node.IsSequence()) {
        for (const auto& entry : node) {
            ids.emplace_back(entry.as<std::uint64_t>());
        }
    }
    return ids;
}

} // namespace

void Emit(YAML::Emitter& out, const AssetMetadata& metadata) {
    out << YAML::BeginMap;
    out << YAML::Key << "Asset" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ID" << YAML::Value << metadata.id.Value();
    out << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.type);
    out << YAML::Key << "Name" << YAML::Value << metadata.name;
    out << YAML::Key << "ImporterVersion" << YAML::Value << metadata.importerVersion;
    out << YAML::Key << "CookerVersion" << YAML::Value << metadata.cookerVersion;
    out << YAML::Key << "RawPath" << YAML::Value << metadata.rawPath.generic_string();
    out << YAML::Key << "CookedPath" << YAML::Value << metadata.cookedPath.generic_string();
    out << YAML::Key << "RawFileTimestamp" << YAML::Value << metadata.rawFileTimestamp;
    out << YAML::Key << "RawFileHash" << YAML::Value << metadata.rawFileHash;
    out << YAML::Key << "ImportedTimestamp" << YAML::Value << metadata.importedTimestamp;
    out << YAML::Key << "CookedTimestamp" << YAML::Value << metadata.cookedTimestamp;
    EmitIdList(out, "Dependencies", metadata.dependencies);
    EmitIdList(out, "Dependents", metadata.dependents);
    out << YAML::Key << "Dirty" << YAML::Value << metadata.dirty;
    out << YAML::Key << "Missing" << YAML::Value << metadata.missing;
    out << YAML::Key << "Cooked" << YAML::Value << metadata.cooked;
    out << YAML::EndMap;
    out << YAML::EndMap;
}

bool Read(const YAML::Node& node, AssetMetadata& out) {
    const YAML::Node asset = node["Asset"];
    if (!asset || !asset.IsMap()) {
        return false;
    }
    if (asset["ID"]) {
        out.id = AssetID(asset["ID"].as<std::uint64_t>());
    }
    if (asset["Type"]) {
        out.type = AssetTypeFromString(asset["Type"].as<std::string>());
    }
    if (asset["Name"]) {
        out.name = asset["Name"].as<std::string>();
    }
    if (asset["ImporterVersion"]) {
        out.importerVersion = asset["ImporterVersion"].as<std::string>();
    }
    if (asset["CookerVersion"]) {
        out.cookerVersion = asset["CookerVersion"].as<std::string>();
    }
    if (asset["RawPath"]) {
        out.rawPath = asset["RawPath"].as<std::string>();
    }
    if (asset["CookedPath"]) {
        out.cookedPath = asset["CookedPath"].as<std::string>();
    }
    if (asset["RawFileTimestamp"]) {
        out.rawFileTimestamp = asset["RawFileTimestamp"].as<std::uint64_t>();
    }
    if (asset["RawFileHash"]) {
        out.rawFileHash = asset["RawFileHash"].as<std::uint64_t>();
    }
    if (asset["ImportedTimestamp"]) {
        out.importedTimestamp = asset["ImportedTimestamp"].as<std::uint64_t>();
    }
    if (asset["CookedTimestamp"]) {
        out.cookedTimestamp = asset["CookedTimestamp"].as<std::uint64_t>();
    }
    out.dependencies = ReadIdList(asset["Dependencies"]);
    out.dependents = ReadIdList(asset["Dependents"]);
    if (asset["Dirty"]) {
        out.dirty = asset["Dirty"].as<bool>();
    }
    if (asset["Missing"]) {
        out.missing = asset["Missing"].as<bool>();
    }
    if (asset["Cooked"]) {
        out.cooked = asset["Cooked"].as<bool>();
    }
    return true;
}

} // namespace MetadataYaml
} // namespace Hockey

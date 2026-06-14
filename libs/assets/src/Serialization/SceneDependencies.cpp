#include "Serialization/SceneDependencies.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"

#include <algorithm>
#include <cstdint>

#include <yaml-cpp/yaml.h>

namespace Hockey {
namespace SceneDependencies {

namespace {

void Walk(const YAML::Node& node, const AssetDatabase& database, std::vector<AssetID>& out) {
    switch (node.Type()) {
    case YAML::NodeType::Scalar: {
        // Only plain unsigned integers can be asset ids; skip anything with a
        // sign, decimal point, or non-digit characters.
        const std::string& text = node.Scalar();
        if (text.empty() ||
            !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
            return;
        }
        uint64_t value = 0;
        try {
            value = std::stoull(text);
        } catch (...) {
            return;
        }
        const AssetID id(value);
        if (id.IsValid() && database.Find(id) != nullptr && std::find(out.begin(), out.end(), id) == out.end()) {
            out.push_back(id);
        }
        break;
    }
    case YAML::NodeType::Sequence:
        for (const YAML::Node& child : node) {
            Walk(child, database, out);
        }
        break;
    case YAML::NodeType::Map:
        for (const auto& entry : node) {
            Walk(entry.second, database, out);
        }
        break;
    default:
        break;
    }
}

} // namespace

std::vector<AssetID> Collect(const std::string& yamlText, const AssetDatabase& database) {
    std::vector<AssetID> deps;
    YAML::Node root;
    try {
        root = YAML::Load(yamlText);
    } catch (const YAML::Exception&) {
        return deps;
    }
    Walk(root, database, deps);
    return deps;
}

Status Validate(const std::string& yamlText) {
    try {
        YAML::Node root = YAML::Load(yamlText);
        (void)root;
    } catch (const YAML::Exception& e) {
        return Status::Fail(std::string("invalid YAML: ") + e.what());
    }
    return Status::Ok();
}

} // namespace SceneDependencies
} // namespace Hockey

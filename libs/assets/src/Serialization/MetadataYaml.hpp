#pragma once
#include <yaml-cpp/yaml.h>

#include "Hockey/Assets/AssetMetadata.hpp"

// Internal (library-private) helpers shared by the sidecar and database
// serializers so the on-disk field layout is defined in exactly one place.
namespace Hockey {
namespace MetadataYaml {

// Emits a full map of the form { Asset: { ...fields... } }.
void Emit(YAML::Emitter& out, const AssetMetadata& metadata);

// Reads from a node that contains an "Asset" map. Returns false if absent.
bool Read(const YAML::Node& node, AssetMetadata& out);

} // namespace MetadataYaml
} // namespace Hockey

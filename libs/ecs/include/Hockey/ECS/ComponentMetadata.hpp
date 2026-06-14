#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

// Editable field kinds the future Unity-style inspector will understand.
enum class FieldType { Bool, Int, Float, String, Vec2, Vec3, Vec4, Enum, UUID, Path, AssetRef };

// Reflection-lite description of a single editable field on a component.
struct FieldMetadata {
    std::string name;
    std::string displayName;
    FieldType type = FieldType::Float;

    std::size_t offset = 0;

    float minFloat = 0.0f;
    float maxFloat = 0.0f;
    float speed = 0.1f;

    int minInt = 0;
    int maxInt = 0;

    std::vector<std::string> enumNames;
    std::vector<int> enumValues;

    // For FieldType::AssetRef: the asset category this field accepts (e.g.
    // "Mesh", "Material", "Texture"). Editors map this to AssetType. Kept as a
    // string so the ECS stays independent of hockey_assets.
    std::string assetTypeName;

    bool readOnly = false;
};

// Describes a component type for editors: its fields plus type-erased
// has/add/remove operations so UI code never needs the concrete type.
struct ComponentMetadata {
    std::string name;
    std::string displayName;

    std::vector<FieldMetadata> fields;

    std::function<bool(Entity)> has;
    std::function<void(Entity)> add;
    std::function<void(Entity)> remove;
    // Type-erased pointer to the component instance on an entity (or nullptr when
    // absent). Combined with FieldMetadata::offset this lets editor UI read and
    // write fields without compile-time knowledge of the component type.
    std::function<void*(Entity)> getData;

    bool addable = true;   // shown in an "Add Component" menu
    bool removable = true; // can be removed from an entity
};

const char* FieldTypeToString(FieldType type);

} // namespace Hockey

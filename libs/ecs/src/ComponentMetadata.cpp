#include "Hockey/ECS/ComponentMetadata.hpp"

namespace Hockey {

const char* FieldTypeToString(FieldType type) {
    switch (type) {
    case FieldType::Bool:
        return "Bool";
    case FieldType::Int:
        return "Int";
    case FieldType::Float:
        return "Float";
    case FieldType::String:
        return "String";
    case FieldType::Vec2:
        return "Vec2";
    case FieldType::Vec3:
        return "Vec3";
    case FieldType::Vec4:
        return "Vec4";
    case FieldType::Enum:
        return "Enum";
    case FieldType::UUID:
        return "UUID";
    case FieldType::Path:
        return "Path";
    case FieldType::AssetRef:
        return "AssetRef";
    }
    return "Unknown";
}

} // namespace Hockey

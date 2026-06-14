#include "Hockey/ECS/RenderComponents.hpp"

namespace Hockey {

std::string LightTypeToString(LightComponent::Type type) {
    switch (type) {
    case LightComponent::Type::Directional:
        return "Directional";
    case LightComponent::Type::Point:
        return "Point";
    case LightComponent::Type::Spot:
        return "Spot";
    }
    return "Directional";
}

LightComponent::Type LightTypeFromString(const std::string& value) {
    if (value == "Point") {
        return LightComponent::Type::Point;
    }
    if (value == "Spot") {
        return LightComponent::Type::Spot;
    }
    return LightComponent::Type::Directional;
}

} // namespace Hockey

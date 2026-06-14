#pragma once

#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/Editor/Tools/EditorTool.hpp"

namespace Hockey {

// Instant tool that creates a single light entity (Directional/Point/Spot) with
// a LightComponent + TransformComponent. One instance is registered per type.
class LightTool : public EditorTool {
public:
    explicit LightTool(LightComponent::Type type) : m_Type(type) {}

    const char* Name() const override;
    const char* Category() const override {
        return "Create";
    }
    bool IsInstant() const override {
        return true;
    }

    void OnSelected(EditorContext& context) override;

private:
    LightComponent::Type m_Type;
};

} // namespace Hockey

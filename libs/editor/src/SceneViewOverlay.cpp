#include "Hockey/Editor/SceneViewOverlay.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <glm/gtc/constants.hpp>

#include <imgui.h>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCamera.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/EditorSettings.hpp"
#include "Hockey/Editor/Selection.hpp"
#include "Hockey/Editor/Tools/ToolManager.hpp"
#include "Hockey/Renderer/Camera.hpp"

namespace Hockey::SceneViewOverlay {

namespace {

constexpr float kMargin = 8.0f;
constexpr float kToolButton = 30.0f;
constexpr float kToolGap = 3.0f;
constexpr float kStripPadding = 4.0f;
constexpr float kControlsHeight = 32.0f;
constexpr float kViewGizmoSize = 94.0f;
constexpr float kIconRadius = 16.0f;

struct Rect {
    glm::vec2 min{0.0f};
    glm::vec2 max{0.0f};

    bool Contains(glm::vec2 p) const {
        return p.x >= min.x && p.y >= min.y && p.x <= max.x && p.y <= max.y;
    }
};

Rect ToolStripRect(glm::vec2 imagePos) {
    const float width = kToolButton + kStripPadding * 2.0f;
    const float height = kToolButton * 4.0f + kToolGap * 3.0f + kStripPadding * 2.0f;
    return {{imagePos.x + kMargin, imagePos.y + kMargin}, {imagePos.x + kMargin + width, imagePos.y + kMargin + height}};
}

Rect ControlsRect(glm::vec2 imagePos, glm::vec2 imageSize) {
    const Rect tools = ToolStripRect(imagePos);
    const float x = tools.max.x + kMargin;
    const float width = std::min(700.0f, std::max(0.0f, imagePos.x + imageSize.x - x - kViewGizmoSize - kMargin * 3.0f));
    return {{x, imagePos.y + kMargin}, {x + width, imagePos.y + kMargin + kControlsHeight}};
}

Rect ViewGizmoRect(glm::vec2 imagePos, glm::vec2 imageSize) {
    const glm::vec2 min{imagePos.x + imageSize.x - kMargin - kViewGizmoSize, imagePos.y + kMargin};
    return {min, min + glm::vec2(kViewGizmoSize)};
}

ImU32 ColorU32(float r, float g, float b, float a = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, a));
}

ImVec2 ToImVec2(glm::vec2 v) {
    return ImVec2(v.x, v.y);
}

glm::vec2 ToGlm(ImVec2 v) {
    return {v.x, v.y};
}

bool IsCameraOrLight(Entity entity) {
    return entity.HasComponent<CameraComponent>() || entity.HasComponent<LightComponent>();
}

const char* ToolNameForIndex(int index) {
    switch (index) {
    case 0:
        return "Select";
    case 1:
        return "Move";
    case 2:
        return "Rotate";
    case 3:
        return "Scale";
    default:
        return "";
    }
}

void Tooltip(const char* text) {
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", text);
    }
}

bool IconButton(const char* id, glm::vec2 pos, glm::vec2 size, bool active, const char* tooltip) {
    ImGui::SetCursorScreenPos(ToImVec2(pos));
    ImGui::InvisibleButton(id, ToImVec2(size));
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    Tooltip(tooltip);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImU32 bg = active ? ColorU32(0.24f, 0.43f, 0.72f) : hovered ? ColorU32(0.25f, 0.25f, 0.28f, 0.96f)
                                                                       : ColorU32(0.16f, 0.16f, 0.18f, 0.92f);
    draw->AddRectFilled(ToImVec2(pos), ToImVec2(pos + size), bg, 3.0f);
    draw->AddRect(ToImVec2(pos), ToImVec2(pos + size), ColorU32(0.04f, 0.04f, 0.05f, 0.9f), 3.0f);
    return clicked;
}

void DrawToolIcon(ImDrawList* draw, int index, glm::vec2 pos, glm::vec2 size) {
    const ImU32 c = ColorU32(0.88f, 0.9f, 0.92f);
    const glm::vec2 center = pos + size * 0.5f;
    const float r = size.x * 0.32f;

    switch (index) {
    case 0: {
        const ImVec2 a = ToImVec2(center + glm::vec2(-6.0f, -9.0f));
        const ImVec2 b = ToImVec2(center + glm::vec2(-2.0f, 8.0f));
        const ImVec2 d = ToImVec2(center + glm::vec2(4.0f, 2.0f));
        draw->AddTriangleFilled(a, b, d, c);
        draw->AddLine(ToImVec2(center + glm::vec2(0.0f, 5.0f)), ToImVec2(center + glm::vec2(7.0f, 11.0f)), c, 2.0f);
        break;
    }
    case 1:
        draw->AddLine(ToImVec2(center + glm::vec2(-r, 0.0f)), ToImVec2(center + glm::vec2(r, 0.0f)), c, 2.0f);
        draw->AddLine(ToImVec2(center + glm::vec2(0.0f, -r)), ToImVec2(center + glm::vec2(0.0f, r)), c, 2.0f);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(r + 4.0f, 0.0f)), ToImVec2(center + glm::vec2(r - 3.0f, -4.0f)),
                                ToImVec2(center + glm::vec2(r - 3.0f, 4.0f)), c);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(-r - 4.0f, 0.0f)), ToImVec2(center + glm::vec2(-r + 3.0f, -4.0f)),
                                ToImVec2(center + glm::vec2(-r + 3.0f, 4.0f)), c);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(0.0f, -r - 4.0f)), ToImVec2(center + glm::vec2(-4.0f, -r + 3.0f)),
                                ToImVec2(center + glm::vec2(4.0f, -r + 3.0f)), c);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(0.0f, r + 4.0f)), ToImVec2(center + glm::vec2(-4.0f, r - 3.0f)),
                                ToImVec2(center + glm::vec2(4.0f, r - 3.0f)), c);
        break;
    case 2:
        draw->PathArcTo(ToImVec2(center), r, 0.25f * glm::pi<float>(), 1.75f * glm::pi<float>(), 24);
        draw->PathStroke(c, 0, 2.0f);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(r * 0.8f, -r * 0.65f)),
                                ToImVec2(center + glm::vec2(r * 1.0f, -r * 0.12f)),
                                ToImVec2(center + glm::vec2(r * 0.45f, -r * 0.25f)), c);
        break;
    case 3:
        draw->AddRect(ToImVec2(center - glm::vec2(r * 0.65f)), ToImVec2(center + glm::vec2(r * 0.65f)), c, 1.0f, 0, 2.0f);
        draw->AddLine(ToImVec2(center + glm::vec2(-r, r)), ToImVec2(center + glm::vec2(r, -r)), c, 2.0f);
        draw->AddTriangleFilled(ToImVec2(center + glm::vec2(r + 3.0f, -r - 3.0f)),
                                ToImVec2(center + glm::vec2(r - 4.0f, -r - 1.0f)),
                                ToImVec2(center + glm::vec2(r + 1.0f, -r + 4.0f)), c);
        break;
    }
}

bool ToggleTextButton(const char* label, bool active, const char* tooltip) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    const bool clicked = ImGui::Button(label);
    if (active) {
        ImGui::PopStyleColor();
    }
    Tooltip(tooltip);
    return clicked;
}

float& ActiveSnapValue(EditorContext& context) {
    switch (context.gizmoOperation) {
    case GizmoOperation::Rotate:
        return context.settings.rotateSnapDegrees;
    case GizmoOperation::Scale:
        return context.settings.scaleSnap;
    case GizmoOperation::Translate:
    case GizmoOperation::None:
    default:
        return context.settings.moveSnap;
    }
}

void DrawToolStrip(EditorContext& context, glm::vec2 imagePos, SceneViewOverlayResult& result) {
    const Rect rect = ToolStripRect(imagePos);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.08f, 0.08f, 0.09f, 0.88f), 4.0f);
    draw->AddRect(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.0f, 0.0f, 0.0f, 0.65f), 4.0f);

    for (int i = 0; i < 4; ++i) {
        const char* toolName = ToolNameForIndex(i);
        const EditorTool* tool = context.toolManager.Find(toolName);
        const bool active = tool != nullptr && context.toolManager.IsActive(tool);
        const glm::vec2 pos{rect.min.x + kStripPadding, rect.min.y + kStripPadding + static_cast<float>(i) * (kToolButton + kToolGap)};
        if (IconButton(toolName, pos, {kToolButton, kToolButton}, active, toolName)) {
            context.toolManager.Activate(toolName, context);
            result.capturedMouse = true;
        }
        result.hovered = result.hovered || ImGui::IsItemHovered();
        DrawToolIcon(draw, i, pos, {kToolButton, kToolButton});
    }
}

void DrawControls(EditorContext& context, glm::vec2 imagePos, glm::vec2 imageSize, bool hasActiveGameCamera,
                  const CameraRenderData& gameCamera, bool& followGameCamera, SceneViewOverlayResult& result) {
    context.physicsView.visualizationOptionsOpen = false;

    const Rect rect = ControlsRect(imagePos, imageSize);
    if (rect.max.x <= rect.min.x + 32.0f) {
        return;
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.08f, 0.08f, 0.09f, 0.82f), 4.0f);
    draw->AddRect(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.0f, 0.0f, 0.0f, 0.55f), 4.0f);

    ImGui::SetCursorScreenPos(ToImVec2(rect.min + glm::vec2(6.0f, 4.0f)));
    ImGui::BeginGroup();
    bool changed = false;

    const bool local = context.transformSpace == TransformSpace::Local;
    if (ToggleTextButton("Local", local, "Use local transform axes")) {
        context.transformSpace = TransformSpace::Local;
    }
    ImGui::SameLine(0.0f, 3.0f);
    if (ToggleTextButton("Global", !local, "Use world transform axes")) {
        context.transformSpace = TransformSpace::World;
    }
    ImGui::SameLine();

    changed |= ToggleTextButton("Grid", context.settings.showGrid, "Show or hide the Scene View grid");
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        context.settings.showGrid = !context.settings.showGrid;
    }
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::SetNextItemWidth(56.0f);
    changed |= ImGui::DragFloat("##scene_grid_spacing", &context.settings.gridSpacing, 0.05f, 0.05f, 100.0f, "%.2f");
    Tooltip("Grid spacing");
    ImGui::SameLine();

    changed |= ToggleTextButton("Snap", context.settings.snapEnabled, "Enable transform snapping");
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        context.settings.snapEnabled = !context.settings.snapEnabled;
    }
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::SetNextItemWidth(56.0f);
    float& snap = ActiveSnapValue(context);
    changed |= ImGui::DragFloat("##scene_snap_value", &snap, 0.01f, 0.001f, 180.0f, "%.2f");
    Tooltip("Active transform snap value");
    ImGui::SameLine();

    ImGui::BeginDisabled(!hasActiveGameCamera);
    if (ImGui::Button("Align") && hasActiveGameCamera) {
        context.editorCamera.SetFromRenderData(gameCamera);
        followGameCamera = false;
    }
    Tooltip("Align the Scene View to the primary game camera");
    ImGui::SameLine(0.0f, 4.0f);
    bool follow = followGameCamera;
    if (ImGui::Checkbox("Follow", &follow)) {
        followGameCamera = follow;
    }
    Tooltip("Follow the primary game camera");
    ImGui::EndDisabled();
    ImGui::SameLine(0.0f, 4.0f);

    const bool physicsActive = context.physicsView.showColliders || context.physicsView.showTriggers ||
                               context.physicsView.showBodyCenters || context.physicsView.showContacts;
    if (ToggleTextButton("Physics", physicsActive, "Scene View physics visualization options")) {
        ImGui::OpenPopup("##scene_physics_visualization");
    }
    if (ImGui::BeginPopup("##scene_physics_visualization")) {
        context.physicsView.visualizationOptionsOpen = true;
        ImGui::TextUnformatted("Visualization");
        ImGui::Separator();
        ImGui::Checkbox("Show Colliders", &context.physicsView.showColliders);
        ImGui::Checkbox("Show Triggers", &context.physicsView.showTriggers);
        ImGui::Checkbox("Show Body Centers", &context.physicsView.showBodyCenters);
        ImGui::Checkbox("Show Contacts", &context.physicsView.showContacts);
        const bool popupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        result.hovered = result.hovered || popupHovered;
        result.capturedMouse =
            result.capturedMouse || (popupHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left));
        ImGui::EndPopup();
    }

    ImGui::EndGroup();

    if (changed) {
        context.settings.Save(EditorSettings::DefaultPath());
    }
    result.hovered = result.hovered || rect.Contains(ToGlm(ImGui::GetIO().MousePos));
    result.capturedMouse = result.capturedMouse || (result.hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left));
}

void DrawCameraIcon(ImDrawList* draw, glm::vec2 center, ImU32 color) {
    const glm::vec2 bodyMin = center + glm::vec2(-10.0f, -6.0f);
    const glm::vec2 bodyMax = center + glm::vec2(4.0f, 6.0f);
    draw->AddRectFilled(ToImVec2(bodyMin), ToImVec2(bodyMax), color, 2.0f);
    draw->AddTriangleFilled(ToImVec2(center + glm::vec2(4.0f, -4.0f)), ToImVec2(center + glm::vec2(13.0f, -9.0f)),
                            ToImVec2(center + glm::vec2(13.0f, 9.0f)), color);
    draw->AddCircleFilled(ToImVec2(center + glm::vec2(-3.0f, 0.0f)), 3.0f, ColorU32(0.08f, 0.08f, 0.09f, 0.9f));
}

void DrawPointLightIcon(ImDrawList* draw, glm::vec2 center, ImU32 color) {
    draw->AddCircleFilled(ToImVec2(center), 6.0f, color);
    for (int i = 0; i < 8; ++i) {
        const float a = static_cast<float>(i) * glm::quarter_pi<float>();
        const glm::vec2 dir{std::cos(a), std::sin(a)};
        draw->AddLine(ToImVec2(center + dir * 9.0f), ToImVec2(center + dir * 14.0f), color, 2.0f);
    }
}

void DrawSpotLightIcon(ImDrawList* draw, glm::vec2 center, ImU32 color) {
    draw->AddCircleFilled(ToImVec2(center + glm::vec2(-7.0f, -5.0f)), 4.0f, color);
    draw->AddTriangleFilled(ToImVec2(center + glm::vec2(-2.0f, -4.0f)), ToImVec2(center + glm::vec2(13.0f, -11.0f)),
                            ToImVec2(center + glm::vec2(13.0f, 8.0f)), ColorU32(1.0f, 0.86f, 0.28f, 0.55f));
    draw->AddLine(ToImVec2(center + glm::vec2(-2.0f, -4.0f)), ToImVec2(center + glm::vec2(13.0f, -11.0f)), color, 1.5f);
    draw->AddLine(ToImVec2(center + glm::vec2(-2.0f, -4.0f)), ToImVec2(center + glm::vec2(13.0f, 8.0f)), color, 1.5f);
}

void DrawDirectionalLightIcon(ImDrawList* draw, glm::vec2 center, ImU32 color) {
    for (int i = 0; i < 4; ++i) {
        const float y = -9.0f + static_cast<float>(i) * 6.0f;
        draw->AddLine(ToImVec2(center + glm::vec2(-12.0f, y - 5.0f)), ToImVec2(center + glm::vec2(8.0f, y + 5.0f)),
                      color, 2.0f);
    }
    draw->AddCircleFilled(ToImVec2(center + glm::vec2(10.0f, 0.0f)), 4.0f, color);
}

void DrawSceneIcons(EditorContext& context, Scene& scene, const CameraRenderData& camera, glm::vec2 imagePos,
                    glm::vec2 imageSize, SceneViewOverlayResult& result) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const glm::vec2 mousePixels = ToGlm(mouse) - imagePos;
    const UUID picked = PickIcon(scene, camera, imageSize, mousePixels, kIconRadius);
    const bool click = picked.IsValid() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    if (click) {
        if (ImGui::GetIO().KeyCtrl) {
            context.selection.Toggle(picked);
        } else {
            context.selection.Select(picked);
        }
        result.capturedMouse = true;
    }

    for (Entity entity : scene.GetAllEntities()) {
        if (!entity.HasComponent<TransformComponent>() || !IsCameraOrLight(entity)) {
            continue;
        }
        const glm::vec3 position(scene.GetWorldTransform(entity)[3]);
        const SceneViewProjection projection = ProjectWorldToViewportPixels(camera, position, imageSize);
        if (!projection.visible) {
            continue;
        }

        const glm::vec2 center = imagePos + projection.pixels;
        const bool hovered = picked == entity.GetUUID();
        const bool selected = context.selection.IsSelected(entity.GetUUID());
        const ImU32 outline = selected ? ColorU32(0.28f, 0.55f, 1.0f) : hovered ? ColorU32(1.0f, 1.0f, 1.0f)
                                                                              : ColorU32(0.0f, 0.0f, 0.0f, 0.55f);
        draw->AddCircleFilled(ToImVec2(center), kIconRadius, ColorU32(0.04f, 0.04f, 0.05f, 0.38f));
        draw->AddCircle(ToImVec2(center), kIconRadius, outline, 24, selected ? 2.2f : 1.2f);

        if (entity.HasComponent<CameraComponent>()) {
            DrawCameraIcon(draw, center, ColorU32(0.9f, 0.95f, 1.0f));
        }
        if (entity.HasComponent<LightComponent>()) {
            const LightComponent& light = entity.GetComponent<LightComponent>();
            const ImU32 lightColor = ColorU32(std::clamp(light.color.r, 0.35f, 1.0f),
                                              std::clamp(light.color.g, 0.35f, 1.0f),
                                              std::clamp(light.color.b, 0.18f, 1.0f));
            switch (light.type) {
            case LightComponent::Type::Directional:
                DrawDirectionalLightIcon(draw, center, lightColor);
                break;
            case LightComponent::Type::Spot:
                DrawSpotLightIcon(draw, center, lightColor);
                break;
            case LightComponent::Type::Point:
                DrawPointLightIcon(draw, center, lightColor);
                break;
            }
        }
    }

    result.hovered = result.hovered || picked.IsValid();
}

SceneViewAxis AxisFromHandle(int handle) {
    switch (handle) {
    case 0:
        return SceneViewAxis::PositiveX;
    case 1:
        return SceneViewAxis::NegativeX;
    case 2:
        return SceneViewAxis::PositiveY;
    case 3:
        return SceneViewAxis::NegativeY;
    case 4:
        return SceneViewAxis::PositiveZ;
    case 5:
        return SceneViewAxis::NegativeZ;
    default:
        return SceneViewAxis::Isometric;
    }
}

void DrawViewGizmo(EditorContext& context, const CameraRenderData& camera, glm::vec2 imagePos, glm::vec2 imageSize,
                   bool& followGameCamera, SceneViewOverlayResult& result) {
    const Rect rect = ViewGizmoRect(imagePos, imageSize);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.06f, 0.06f, 0.07f, 0.44f), 5.0f);
    draw->AddRect(ToImVec2(rect.min), ToImVec2(rect.max), ColorU32(0.0f, 0.0f, 0.0f, 0.55f), 5.0f);

    const glm::vec2 center = (rect.min + rect.max) * 0.5f;
    const glm::mat4 view = camera.view;
    struct AxisDraw {
        glm::vec3 axis;
        const char* label;
        ImU32 color;
    };
    const std::array<AxisDraw, 6> axes{{
        {{1.0f, 0.0f, 0.0f}, "X", ColorU32(0.88f, 0.22f, 0.22f)},
        {{-1.0f, 0.0f, 0.0f}, "-X", ColorU32(0.52f, 0.16f, 0.16f)},
        {{0.0f, 1.0f, 0.0f}, "Y", ColorU32(0.35f, 0.9f, 0.22f)},
        {{0.0f, -1.0f, 0.0f}, "-Y", ColorU32(0.18f, 0.52f, 0.14f)},
        {{0.0f, 0.0f, 1.0f}, "Z", ColorU32(0.26f, 0.45f, 1.0f)},
        {{0.0f, 0.0f, -1.0f}, "-Z", ColorU32(0.14f, 0.25f, 0.58f)},
    }};

    const glm::vec2 mouse = ToGlm(ImGui::GetIO().MousePos);
    int hoveredHandle = -1;
    std::array<glm::vec2, 6> ends{};
    for (int i = 0; i < 6; ++i) {
        const glm::vec4 v = view * glm::vec4(axes[static_cast<std::size_t>(i)].axis, 0.0f);
        glm::vec2 dir{v.x, -v.y};
        if (glm::dot(dir, dir) < 1e-4f) {
            dir = {0.0f, -1.0f};
        } else {
            dir = glm::normalize(dir);
        }
        ends[static_cast<std::size_t>(i)] = center + dir * 31.0f;
        if (glm::length(mouse - ends[static_cast<std::size_t>(i)]) <= 11.0f) {
            hoveredHandle = i;
        }
    }

    const bool centerHovered = glm::length(mouse - center) <= 13.0f;
    if (centerHovered) {
        hoveredHandle = 6;
    }
    if (hoveredHandle >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        context.editorCamera.SetViewDirection(ForwardForAxis(AxisFromHandle(hoveredHandle)));
        followGameCamera = false;
        result.capturedMouse = true;
    }

    for (int i = 0; i < 6; ++i) {
        const AxisDraw& axis = axes[static_cast<std::size_t>(i)];
        const glm::vec2 end = ends[static_cast<std::size_t>(i)];
        draw->AddLine(ToImVec2(center), ToImVec2(end), axis.color, i % 2 == 0 ? 2.0f : 1.35f);
        draw->AddCircleFilled(ToImVec2(end), hoveredHandle == i ? 8.0f : 6.0f, axis.color);
        const float labelOffset = axis.label[0] == '-' ? -8.0f : -4.0f;
        draw->AddText(ToImVec2(end + glm::vec2(labelOffset, -6.0f)), ColorU32(1.0f, 1.0f, 1.0f), axis.label);
    }
    draw->AddCircleFilled(ToImVec2(center), centerHovered ? 10.0f : 8.0f, ColorU32(0.75f, 0.75f, 0.78f));
    draw->AddText(ToImVec2(center + glm::vec2(-8.0f, 12.0f)), ColorU32(0.84f, 0.84f, 0.86f), "Iso");

    result.hovered = result.hovered || rect.Contains(mouse);
}

} // namespace

SceneViewProjection ProjectWorldToViewportPixels(const CameraRenderData& camera, glm::vec3 worldPosition,
                                                 glm::vec2 viewportPixels) {
    SceneViewProjection result;
    if (viewportPixels.x <= 0.0f || viewportPixels.y <= 0.0f) {
        return result;
    }

    const glm::vec4 clip = camera.projection * camera.view * glm::vec4(worldPosition, 1.0f);
    if (clip.w <= 1e-5f) {
        return result;
    }
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f || ndc.z < 0.0f || ndc.z > 1.0f) {
        return result;
    }

    result.visible = true;
    result.pixels = {(ndc.x * 0.5f + 0.5f) * viewportPixels.x, (ndc.y * 0.5f + 0.5f) * viewportPixels.y};
    result.depth = ndc.z;
    return result;
}

glm::vec3 ForwardForAxis(SceneViewAxis axis) {
    switch (axis) {
    case SceneViewAxis::PositiveX:
        return {-1.0f, 0.0f, 0.0f};
    case SceneViewAxis::NegativeX:
        return {1.0f, 0.0f, 0.0f};
    case SceneViewAxis::PositiveY:
        return {0.0f, -1.0f, 0.0f};
    case SceneViewAxis::NegativeY:
        return {0.0f, 1.0f, 0.0f};
    case SceneViewAxis::PositiveZ:
        return {0.0f, 0.0f, -1.0f};
    case SceneViewAxis::NegativeZ:
        return {0.0f, 0.0f, 1.0f};
    case SceneViewAxis::Isometric:
    default:
        return glm::normalize(glm::vec3(-1.0f, -0.65f, -1.0f));
    }
}

UUID PickIcon(Scene& scene, const CameraRenderData& camera, glm::vec2 viewportPixels, glm::vec2 mousePixels,
              float radiusPixels) {
    if (radiusPixels <= 0.0f) {
        return UUID{0};
    }

    UUID best{0};
    float bestDistanceSq = radiusPixels * radiusPixels;
    float bestDepth = std::numeric_limits<float>::max();
    for (Entity entity : scene.GetAllEntities()) {
        if (!entity.HasComponent<TransformComponent>() || !IsCameraOrLight(entity)) {
            continue;
        }
        const SceneViewProjection projected =
            ProjectWorldToViewportPixels(camera, glm::vec3(scene.GetWorldTransform(entity)[3]), viewportPixels);
        if (!projected.visible) {
            continue;
        }

        const glm::vec2 delta = projected.pixels - mousePixels;
        const float distanceSq = glm::dot(delta, delta);
        if (distanceSq < bestDistanceSq || (std::abs(distanceSq - bestDistanceSq) <= 1e-4f && projected.depth < bestDepth)) {
            bestDistanceSq = distanceSq;
            bestDepth = projected.depth;
            best = entity.GetUUID();
        }
    }
    return best;
}

bool ContainsControls(glm::vec2 imagePos, glm::vec2 imageSize, glm::vec2 mousePos) {
    return ToolStripRect(imagePos).Contains(mousePos) || ControlsRect(imagePos, imageSize).Contains(mousePos) ||
           ViewGizmoRect(imagePos, imageSize).Contains(mousePos);
}

SceneViewOverlayResult Draw(EditorContext& context, Scene& scene, const CameraRenderData& sceneCamera,
                            glm::vec2 imagePos, glm::vec2 imageSize, bool hasActiveGameCamera,
                            const CameraRenderData& gameCamera, bool& followGameCamera) {
    SceneViewOverlayResult result;
    DrawSceneIcons(context, scene, sceneCamera, imagePos, imageSize, result);
    DrawToolStrip(context, imagePos, result);
    DrawControls(context, imagePos, imageSize, hasActiveGameCamera, gameCamera, followGameCamera, result);
    DrawViewGizmo(context, sceneCamera, imagePos, imageSize, followGameCamera, result);
    return result;
}

} // namespace Hockey::SceneViewOverlay

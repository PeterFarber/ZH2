#include "Hockey/Editor/ImGui/EditorIcons.hpp"

#include <string_view>

#include <IconsFontAwesome.h>
#include <imgui.h>

#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

namespace Hockey {
namespace {

std::string_view VisibleIdText(const char* id) {
    if (id == nullptr || id[0] == '\0') {
        return {};
    }
    const std::string_view text{id};
    const std::size_t marker = text.find("##");
    if (marker == std::string_view::npos) {
        return text;
    }
    return text.substr(0, marker);
}

std::string IconButtonLabel(EditorIcon icon, const char* id) {
    const char* glyph = EditorIconGlyph(icon);
    const std::string_view fallback = VisibleIdText(id);

    std::string label = (glyph != nullptr && glyph[0] != '\0') ? std::string{glyph} : std::string{fallback};
    if (label.empty()) {
        label = "?";
    }
    if (id != nullptr && id[0] != '\0') {
        label += "##";
        label += id;
    }
    return label;
}

} // namespace

const char* EditorIconGlyph(EditorIcon icon) {
    switch (icon) {
    case EditorIcon::Add:
        return ICON_FA_PLUS;
    case EditorIcon::Delete:
        return ICON_FA_TRASH;
    case EditorIcon::Edit:
        return ICON_FA_PEN;
    case EditorIcon::Save:
        return ICON_FA_FLOPPY_DISK;
    case EditorIcon::Search:
        return ICON_FA_MAGNIFYING_GLASS;
    case EditorIcon::Clear:
        return ICON_FA_XMARK;
    case EditorIcon::Refresh:
        return ICON_FA_ROTATE;
    case EditorIcon::Import:
        return ICON_FA_FILE_IMPORT;
    case EditorIcon::Cook:
        return ICON_FA_FIRE;
    case EditorIcon::Validate:
        return ICON_FA_CIRCLE_CHECK;
    case EditorIcon::Copy:
        return ICON_FA_COPY;
    case EditorIcon::Paste:
        return ICON_FA_PASTE;
    case EditorIcon::Undo:
        return ICON_FA_ARROW_ROTATE_LEFT;
    case EditorIcon::Redo:
        return ICON_FA_ARROW_ROTATE_RIGHT;
    case EditorIcon::Play:
        return ICON_FA_PLAY;
    case EditorIcon::Pause:
        return ICON_FA_PAUSE;
    case EditorIcon::Step:
        return ICON_FA_FORWARD_STEP;
    case EditorIcon::Folder:
        return ICON_FA_FOLDER;
    case EditorIcon::File:
        return ICON_FA_FILE;
    case EditorIcon::Scene:
        return ICON_FA_MAP;
    case EditorIcon::Prefab:
        return ICON_FA_CUBE;
    case EditorIcon::Material:
        return ICON_FA_PALETTE;
    case EditorIcon::Texture:
        return ICON_FA_IMAGE;
    case EditorIcon::Model:
        return ICON_FA_CUBES;
    case EditorIcon::Mesh:
        return ICON_FA_VECTOR_SQUARE;
    case EditorIcon::Shader:
        return ICON_FA_CODE;
    case EditorIcon::Visible:
        return ICON_FA_EYE;
    case EditorIcon::Hidden:
        return ICON_FA_EYE_SLASH;
    case EditorIcon::Pickable:
        return ICON_FA_LOCK_OPEN;
    case EditorIcon::Locked:
        return ICON_FA_LOCK;
    case EditorIcon::Select:
        return ICON_FA_OBJECT_GROUP;
    case EditorIcon::Move:
        return ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT;
    case EditorIcon::Rotate:
        return ICON_FA_ROTATE;
    case EditorIcon::Scale:
        return ICON_FA_EXPAND;
    case EditorIcon::Focus:
        return ICON_FA_BULLSEYE;
    case EditorIcon::Settings:
        return ICON_FA_GEAR;
    case EditorIcon::Camera:
        return ICON_FA_CAMERA;
    case EditorIcon::Light:
        return ICON_FA_LIGHTBULB;
    case EditorIcon::Grid:
        return ICON_FA_BORDER_ALL;
    case EditorIcon::Snap:
        return ICON_FA_MAGNET;
    case EditorIcon::Physics:
        return ICON_FA_ATOM;
    case EditorIcon::Spawn:
        return ICON_FA_LOCATION_DOT;
    case EditorIcon::Faceoff:
        return ICON_FA_CIRCLE_DOT;
    case EditorIcon::Puck:
        return ICON_FA_HOCKEY_PUCK;
    case EditorIcon::Goal:
        return ICON_FA_FLAG_CHECKERED;
    case EditorIcon::Rink:
        return ICON_FA_BORDER_ALL;
    case EditorIcon::PlayArea:
        return ICON_FA_VECTOR_SQUARE;
    case EditorIcon::CameraRig:
        return ICON_FA_VIDEO;
    case EditorIcon::Warning:
        return ICON_FA_TRIANGLE_EXCLAMATION;
    case EditorIcon::Error:
        return ICON_FA_CIRCLE_EXCLAMATION;
    case EditorIcon::Info:
        return ICON_FA_CIRCLE_INFO;
    case EditorIcon::None:
        break;
    }
    return "";
}

std::string EditorIconLabel(EditorIcon icon, std::string_view text) {
    const char* glyph = EditorIconGlyph(icon);
    if (glyph == nullptr || glyph[0] == '\0') {
        return std::string{text};
    }
    if (text.empty()) {
        return std::string{glyph};
    }
    std::string label{glyph};
    label += ' ';
    label += text;
    return label;
}

bool EditorIconButton(EditorIcon icon, const char* id, const char* tooltip, const ImVec2& size) {
    const std::string label = IconButtonLabel(icon, id);
    const bool pressed = ImGui::Button(label.c_str(), size);
    EditorTooltip::ForLastItem(tooltip);
    return pressed;
}

bool EditorIconToggleButton(EditorIcon icon, const char* id, bool active, const char* tooltip, const ImVec2& size) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    }

    const bool pressed = EditorIconButton(icon, id, tooltip, size);

    if (active) {
        ImGui::PopStyleColor(3);
    }
    return pressed;
}

EditorIcon EditorIconForAssetType(EditorFileType type) {
    switch (type) {
    case EditorFileType::Folder:
        return EditorIcon::Folder;
    case EditorFileType::Scene:
        return EditorIcon::Scene;
    case EditorFileType::Prefab:
        return EditorIcon::Prefab;
    case EditorFileType::Material:
        return EditorIcon::Material;
    case EditorFileType::UIScreen:
    case EditorFileType::ClientFlow:
        return EditorIcon::File;
    case EditorFileType::UITheme:
        return EditorIcon::Material;
    case EditorFileType::ShaderSource:
    case EditorFileType::ShaderBinary:
        return EditorIcon::Shader;
    case EditorFileType::Image:
        return EditorIcon::Texture;
    case EditorFileType::Model:
        return EditorIcon::Model;
    case EditorFileType::Toml:
    case EditorFileType::Text:
    case EditorFileType::Unknown:
        return EditorIcon::File;
    }
    return EditorIcon::File;
}

} // namespace Hockey

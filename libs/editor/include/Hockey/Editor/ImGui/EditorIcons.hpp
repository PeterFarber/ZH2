#pragma once

#include <string>
#include <string_view>

#include <imgui.h>

#include "Hockey/Editor/Project/FileTypeRegistry.hpp"

namespace Hockey {

enum class EditorIcon {
    None,
    Add,
    Delete,
    Edit,
    Save,
    Search,
    Clear,
    Refresh,
    Import,
    Cook,
    Validate,
    Copy,
    Paste,
    Undo,
    Redo,
    Play,
    Pause,
    Step,
    Folder,
    File,
    Scene,
    Prefab,
    Material,
    Texture,
    Model,
    Mesh,
    Shader,
    Visible,
    Hidden,
    Pickable,
    Locked,
    Select,
    Move,
    Rotate,
    Scale,
    Focus,
    Settings,
    Camera,
    Light,
    Grid,
    Snap,
    Physics,
    Spawn,
    Faceoff,
    Puck,
    Goal,
    Rink,
    PlayArea,
    CameraRig,
    Warning,
    Error,
    Info,
};

const char* EditorIconGlyph(EditorIcon icon);
std::string EditorIconLabel(EditorIcon icon, std::string_view text);
bool EditorIconButton(EditorIcon icon, const char* id, const char* tooltip, const ImVec2& size = ImVec2(0.0f, 0.0f));
bool EditorIconToggleButton(EditorIcon icon,
                            const char* id,
                            bool active,
                            const char* tooltip,
                            const ImVec2& size = ImVec2(0.0f, 0.0f));
EditorIcon EditorIconForAssetType(EditorFileType type);

} // namespace Hockey

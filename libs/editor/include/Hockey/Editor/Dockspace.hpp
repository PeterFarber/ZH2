#pragma once

#include <cstdint>

namespace Hockey {

class EditorContext;

// Canonical ImGui window titles for the editor panels. The dockspace's default
// layout docks windows by these names, and the panels (added in later steps)
// use the exact same titles in ImGui::Begin so they land in the right node.
namespace EditorPanelNames {
inline constexpr const char* kHierarchy = "Hierarchy";
inline constexpr const char* kInspector = "Inspector";
inline constexpr const char* kSceneViewport = "Scene";
inline constexpr const char* kGameViewport = "Game";
inline constexpr const char* kProject = "Project";
inline constexpr const char* kConsole = "Console";
inline constexpr const char* kProperties = "Properties";
inline constexpr const char* kStats = "Stats";
inline constexpr const char* kSceneValidation = "Scene Validation";
inline constexpr const char* kPrefab = "Prefab";
inline constexpr const char* kPhysics = "Physics";
} // namespace EditorPanelNames

// Owns the fullscreen host window and the central dock node that every panel
// docks into. The host window also carries the main menu bar and toolbar, which
// the caller draws between BeginHost() and SubmitDockSpace().
//
// Usage per frame:
//   dockspace.BeginHost();
//   menuBar.Draw(...);     // drawn into the host's menu bar
//   toolbar.Draw(...);     // drawn as a strip under the menu bar
//   dockspace.SubmitDockSpace();
//   dockspace.EndHost();
class Dockspace {
public:
    void BeginHost();
    void SubmitDockSpace();
    void EndHost();

    // Forces the default layout to be rebuilt on the next SubmitDockSpace,
    // discarding any docked/resized state (View > Reset Layout).
    void RequestReset() {
        m_ResetRequested = true;
    }

    std::uint32_t DockspaceId() const {
        return m_DockspaceId;
    }

private:
    void BuildDefaultLayout();

    std::uint32_t m_DockspaceId = 0;
    bool m_FirstFrame = true;
    bool m_ResetRequested = false;
    bool m_HostOpen = false;
};

} // namespace Hockey

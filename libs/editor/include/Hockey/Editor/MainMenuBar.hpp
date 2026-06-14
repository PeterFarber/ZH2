#pragma once

namespace Hockey {

class EditorContext;
class EditorApp;

// Draws the editor's main menu bar inside the dockspace host window. Menu items
// whose backing systems arrive in later Phase 4 steps (selection, undo, tools,
// scene IO) are shown disabled until those steps wire them up.
class MainMenuBar {
public:
    void Draw(EditorContext& ctx, EditorApp& app);

private:
    void DrawFileMenu(EditorContext& ctx, EditorApp& app);
    void DrawEditMenu(EditorContext& ctx, EditorApp& app);
    void DrawGameObjectMenu(EditorContext& ctx, EditorApp& app);
    void DrawComponentMenu(EditorContext& ctx, EditorApp& app);
    void DrawToolsMenu(EditorContext& ctx, EditorApp& app);
    void DrawViewMenu(EditorContext& ctx, EditorApp& app);
    void DrawWindowMenu(EditorContext& ctx, EditorApp& app);
    void DrawHelpMenu(EditorContext& ctx, EditorApp& app);
    void DrawHelpPopups();

    bool m_OpenAbout = false;
    bool m_OpenControls = false;
    bool m_OpenProjectRules = false;
};

} // namespace Hockey

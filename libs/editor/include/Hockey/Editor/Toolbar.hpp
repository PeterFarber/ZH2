#pragma once

namespace Hockey {

class EditorContext;
class EditorApp;

// Draws the toolbar strip beneath the main menu bar: tool buttons, transform
// space / snap / grid toggles, play-mode controls, save/validate, and the
// active scene name + dirty indicator. Tool buttons and play/pause/step that
// depend on later-step systems are shown disabled for now.
class Toolbar {
public:
    // Drawn inside the dockspace host window, between the menu bar and the
    // dock space. Returns the height consumed so the caller can lay out.
    void Draw(EditorContext& ctx, EditorApp& app);
};

} // namespace Hockey

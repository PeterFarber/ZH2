#pragma once

namespace Hockey {

class EditorContext;
class EditorApp;

// Draws the app-level toolbar strip beneath the main menu bar: playtest,
// save/validate, and the active scene name + dirty indicator. Scene View
// authoring controls live inside the Scene viewport overlay.
class Toolbar {
public:
    // Drawn inside the dockspace host window, between the menu bar and the
    // dock space. Returns the height consumed so the caller can lay out.
    void Draw(EditorContext& ctx, EditorApp& app);
};

} // namespace Hockey

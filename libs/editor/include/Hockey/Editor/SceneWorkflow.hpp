#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

class EditorContext;
class Scene;

// Scene lifecycle operations (new/open/save/save-as) plus autosave. These mutate
// the context's active Scene in place (the host owns the Scene object) and reset
// editor state (selection, undo stack, dirty flag) as appropriate. File dialogs
// and prompts live in EditorApp; this type is dialog-free so it is unit-testable.
class SceneWorkflow {
public:
    // Resets the active scene to a fresh Edit-mode scene. Clears selection/undo,
    // clears the active path, and (by default) seeds a default directional light.
    void NewScene(EditorContext& context, bool createDefaults = true);

    // Loads a .scene.yaml into the active scene. On success sets the active path,
    // clears selection/undo, clears dirty, and records the recent scene.
    Status OpenScene(EditorContext& context, const std::filesystem::path& path);

    // Serializes to the active path. Fails if no active path is set.
    Status SaveScene(EditorContext& context);

    // Serializes to 'path' (forcing a .scene.yaml suffix), then makes it active.
    Status SaveSceneAs(EditorContext& context, const std::filesystem::path& path);

    // Per-frame autosave; writes a separate autosave copy when enabled, due, and
    // the scene is dirty. Never clears the dirty flag (autosave is not a save).
    void TickAutosave(EditorContext& context, float deltaTime);

    // Resets the autosave timer (e.g. after an explicit save or scene switch).
    void ResetAutosaveTimer() {
        m_AutosaveTimer = 0.0f;
    }

    static std::filesystem::path AutosaveDirectory();
    static std::filesystem::path AutosavePathFor(const Scene& scene);

    // Appends ".scene.yaml" if 'path' does not already carry that suffix.
    static std::filesystem::path EnsureSceneExtension(const std::filesystem::path& path);

private:
    float m_AutosaveTimer = 0.0f;
};

} // namespace Hockey

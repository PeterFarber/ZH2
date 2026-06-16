#pragma once

#include "Hockey/Editor/Panel.hpp"

namespace Hockey {

// Editor preferences editor over EditorContext::settings. Groups the persisted
// EditorSettings fields (autosave, validation, grid/snap, camera, asset
// pipeline) into collapsing sections and writes the settings TOML whenever a
// value changes. Hidden by default; opened from Edit > Preferences.
class PreferencesPanel : public Panel {
public:
    PreferencesPanel();
    void OnImGui(EditorContext& context) override;
};

} // namespace Hockey

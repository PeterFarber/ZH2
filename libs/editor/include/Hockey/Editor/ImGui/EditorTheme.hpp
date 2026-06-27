#pragma once

namespace Hockey {

// Centralized Dear ImGui styling for the editor. Keeping the look in one place
// makes it easy to tweak and (later) load from data/editor/themes/*.toml.
namespace EditorTheme {

// Applies the default dark editor theme to the current ImGui context. Must be
// called after ImGui::CreateContext().
void ApplyDark(float editorScale = 1.0f);

} // namespace EditorTheme

} // namespace Hockey

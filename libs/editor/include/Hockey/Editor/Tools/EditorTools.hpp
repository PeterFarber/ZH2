#pragma once

namespace Hockey {

class ToolManager;

// Registers the full default tool set (transform tools, lights, hockey markers)
// onto a ToolManager. Shared by EditorApp and the tool tests so both exercise
// the same registration.
void RegisterEditorTools(ToolManager& tools);

} // namespace Hockey

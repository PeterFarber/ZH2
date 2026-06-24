# Phase 4 Plan — Complete Unity-Style Map Editor

File path recommendation:

```text
docs/phase-plans/phase-04-unity-style-editor.md
```

AI instruction:

```text
Read `docs/phase-plans/phase-04-unity-style-editor.md` and implement Phase 4 exactly. Build the complete Unity-style map editor on top of the existing core, ECS, scene, prefab, and Vulkan renderer systems. Do not implement the final asset pipeline, physics simulation, networking, GameNetworkingSockets, hockey gameplay simulation, audio, or animation during this phase. Keep the dedicated server headless and make sure it does not link editor UI code. Work step by step, update CMake whenever files are added, add tests/smoke tests, and keep Windows/Linux compatibility.
```

---

# 0. Phase Summary

Phase 4 builds the complete map editor application and shared editor library.

This phase turns `HockeyMapEditor` into a real Unity-style editor with:

```text
docking layout
main menu
toolbar
hierarchy panel
inspector panel
scene viewport
game viewport
project/assets panel
console panel
settings/stats panels
entity selection
component editing
transform gizmos
scene save/load
prefab creation/instancing
hockey-specific placement tools
editor camera
viewport interaction
undo/redo
clipboard
autosave
editor settings
```

Phase 4 must produce:

```text
hockey_editor
HockeyEditorTests
```

Phase 4 updates:

```text
HockeyMapEditor
hockey_ecs component metadata usage
hockey_renderer editor viewport integration if any bridge work is needed
data/editor
data/config/editor.toml
data/raw/scenes
data/raw/prefabs
```

Phase 4 must not implement:

```text
final asset pipeline
physics simulation
Jolt
networking
GameNetworkingSockets
hockey gameplay simulation
audio
animation
final game UI
```

The editor should be complete enough that later phases can add assets, physics, gameplay, and networking without redesigning the editor.

---

# 1. Architecture Rules

## 1.1 Dependency Direction

Phase 4 adds:

```text
hockey_editor -> hockey_core, hockey_ecs, hockey_renderer
```

Allowed app links:

```text
HockeyMapEditor -> hockey_core, hockey_ecs, hockey_renderer, hockey_editor
HockeyEditorTests -> hockey_core, hockey_ecs, hockey_renderer, hockey_editor
```

Do not change:

```text
HockeyDedicatedServer must not link hockey_editor.
HockeyDedicatedServer must not link ImGui.
HockeyDedicatedServer must not link hockey_renderer.
HockeyGameClient should not link hockey_editor.
```

Allowed:

```text
hockey_editor may depend on renderer because editor viewport uses renderer.
hockey_editor may depend on ECS because it edits scenes/entities/components.
hockey_editor may depend on core because it uses config, paths, logging, input, filesystem.
```

Forbidden:

```text
hockey_editor -> hockey_networking
hockey_editor -> hockey_gameplay
hockey_editor -> hockey_physics
hockey_editor -> final asset pipeline
```

## 1.2 System Ownership

`hockey_editor` owns:

```text
editor UI
docking layout
panels
menus
toolbar
selection state
editor camera
viewport controls
gizmos
scene commands
undo/redo
clipboard
inspector field editing
project panel browsing
console viewer
editor settings
autosave
hockey placement tools
```

`hockey_editor` does not own:

```text
scene storage itself
component storage itself
Vulkan rendering internals
physics simulation
networking
hockey match rules
final asset import/cooking
```

## 1.3 Editor/ECS Boundary

ECS owns:

```text
Entity
Scene
Components
Component metadata
Scene serialization
Prefab serialization
Scene validation
```

Editor uses ECS APIs. Editor should not bypass ECS invariants by directly corrupting EnTT storage.

## 1.4 Editor/Renderer Boundary

Renderer owns:

```text
offscreen render targets
scene rendering
debug draw
GPU resources
viewport render texture
```

Editor owns:

```text
viewport panel UI
camera navigation
viewport resize decision
selection
gizmo manipulation
```

Editor may request renderer resize/render but must not directly own Vulkan objects.

---

# 2. Dependencies

Update `vcpkg.json`.

Add:

```json
"imgui",
"nativefiledialog-extended"
```

Optional if available and approved:

```json
"implot",
"imguizmo"
```

Use `implot` only if stats/graphs are implemented in this phase. If not included, use Dear ImGui tables/text for stats.

Use `imguizmo` for transform gizmos. If unavailable in vcpkg, either:

```text
vendor a small ImGuizmo copy under third_party/imguizmo if project policy allows it
or implement editor transform editing through Inspector + simple viewport handles
```

Do not claim full viewport transform gizmos work unless actual manipulation exists.

Do not add:

```text
Jolt
GameNetworkingSockets
FMOD/Wwise
assimp
full asset pipeline dependencies
```

---

# 3. Target Project Structure

Add:

```text
libs/
  editor/
    CMakeLists.txt
    include/Hockey/Editor/
      EditorApp.hpp
      EditorContext.hpp
      EditorSettings.hpp
      EditorTheme.hpp

      Dockspace.hpp
      MainMenuBar.hpp
      Toolbar.hpp

      Panel.hpp
      PanelManager.hpp

      Panels/
        HierarchyPanel.hpp
        InspectorPanel.hpp
        SceneViewportPanel.hpp
        GameViewportPanel.hpp
        ProjectPanel.hpp
        ConsolePanel.hpp
        PropertiesPanel.hpp
        StatsPanel.hpp
        SceneValidationPanel.hpp
        PrefabPanel.hpp

      Selection.hpp
      EditorCamera.hpp
      EditorCommands.hpp
      UndoRedo.hpp
      Clipboard.hpp

      Inspector/
        ComponentInspector.hpp
        FieldDrawers.hpp
        ComponentAddMenu.hpp

      Gizmos/
        TransformGizmo.hpp
        GridGizmo.hpp
        SelectionOutline.hpp

      Tools/
        EditorTool.hpp
        ToolManager.hpp
        SelectTool.hpp
        MoveTool.hpp
        RotateTool.hpp
        ScaleTool.hpp
        HockeyRinkTool.hpp
        HockeySpawnTool.hpp
        HockeyGoalTool.hpp
        HockeyPuckTool.hpp
        HockeyFaceoffTool.hpp
        HockeyCameraRigTool.hpp
        LightTool.hpp

      Project/
        ProjectBrowser.hpp
        FileTypeRegistry.hpp
        EditorAssetPreview.hpp

      Logging/
        EditorConsoleSink.hpp

      ImGui/
        ImGuiLayer.hpp
        ImGuiRendererBridge.hpp
        ImGuiUtils.hpp

    src/
      EditorApp.cpp
      EditorContext.cpp
      EditorSettings.cpp
      EditorTheme.cpp

      Dockspace.cpp
      MainMenuBar.cpp
      Toolbar.cpp

      Panel.cpp
      PanelManager.cpp

      Panels/
        HierarchyPanel.cpp
        InspectorPanel.cpp
        SceneViewportPanel.cpp
        GameViewportPanel.cpp
        ProjectPanel.cpp
        ConsolePanel.cpp
        PropertiesPanel.cpp
        StatsPanel.cpp
        SceneValidationPanel.cpp
        PrefabPanel.cpp

      Selection.cpp
      EditorCamera.cpp
      EditorCommands.cpp
      UndoRedo.cpp
      Clipboard.cpp

      Inspector/
        ComponentInspector.cpp
        FieldDrawers.cpp
        ComponentAddMenu.cpp

      Gizmos/
        TransformGizmo.cpp
        GridGizmo.cpp
        SelectionOutline.cpp

      Tools/
        EditorTool.cpp
        ToolManager.cpp
        SelectTool.cpp
        MoveTool.cpp
        RotateTool.cpp
        ScaleTool.cpp
        HockeyRinkTool.cpp
        HockeySpawnTool.cpp
        HockeyGoalTool.cpp
        HockeyPuckTool.cpp
        HockeyFaceoffTool.cpp
        HockeyCameraRigTool.cpp
        LightTool.cpp

      Project/
        ProjectBrowser.cpp
        FileTypeRegistry.cpp
        EditorAssetPreview.cpp

      Logging/
        EditorConsoleSink.cpp

      ImGui/
        ImGuiLayer.cpp
        ImGuiRendererBridge.cpp
        ImGuiUtils.cpp

apps/
  editor_tests/
    CMakeLists.txt
    src/
      EditorTestsMain.cpp
      Test.hpp
      EditorSettingsTests.cpp
      SelectionTests.cpp
      UndoRedoTests.cpp
      InspectorFieldTests.cpp
      SceneCommandTests.cpp
      ProjectBrowserTests.cpp
      HockeyToolTests.cpp

data/
  editor/
    layout.ini
    editor_settings.toml
    themes/
      dark.toml
    icons/
      README.md
```

Update `apps/map_editor` to use `hockey_editor`.

Keep the executable name:

```text
HockeyMapEditor
```

---

# 4. CMake Requirements

## 4.1 Root CMake

Add:

```cmake
add_subdirectory(libs/editor)
```

When tests are enabled:

```cmake
add_subdirectory(apps/editor_tests)
```

Order:

```text
libs/core
libs/ecs
libs/renderer
libs/editor
apps
```

## 4.2 `libs/editor/CMakeLists.txt`

Create:

```text
hockey_editor
```

Required links:

```text
hockey_core
hockey_ecs
hockey_renderer
imgui target
nativefiledialog-extended target
imguizmo target or local source if used
implot target if used
```

Do not link:

```text
hockey_gameplay
hockey_networking
hockey_physics
```

## 4.3 App Link Updates

Update:

```text
HockeyMapEditor -> hockey_editor
```

Do not update:

```text
HockeyGameClient -> hockey_editor
HockeyDedicatedServer -> hockey_editor
```

## 4.4 Server Link Guard

Add a test/check or clear CMake assertion/comment verifying:

```text
HockeyDedicatedServer does not link hockey_editor.
HockeyDedicatedServer does not link ImGui.
```

---

# 5. Editor App Integration

## 5.1 Existing Map Editor App

`apps/map_editor` should become a thin executable wrapper.

Responsibilities:

```text
parse command line
initialize core application
create window
initialize renderer
initialize editor layer/app
run editor update/render loop
shutdown editor
shutdown renderer
shutdown core
```

Do not put editor implementation logic in `apps/map_editor`.

Put editor logic in:

```text
libs/editor
```

## 5.2 `EditorApp`

Implement:

```cpp
class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    Status Init(EditorContextCreateInfo createInfo);
    void Shutdown();

    void BeginFrame(float deltaTime);
    void Update(float deltaTime);
    void Render();
    void EndFrame();

    bool WantsQuit() const;

    EditorContext& GetContext();
};
```

## 5.3 `EditorContext`

Implement central editor state:

```cpp
struct EditorContextCreateInfo {
    Window* window = nullptr;
    Renderer* renderer = nullptr;
    Scene* scene = nullptr;
    Config* config = nullptr;
};

class EditorContext {
public:
    Window* window = nullptr;
    Renderer* renderer = nullptr;
    Scene* activeScene = nullptr;

    Selection selection;
    EditorCamera editorCamera;
    PanelManager panelManager;
    ToolManager toolManager;
    UndoRedoStack undoRedo;

    EditorSettings settings;

    std::filesystem::path activeScenePath;
    bool sceneDirty = false;
    bool playMode = false;
    bool simulateMode = false;
};
```

Rules:

```text
EditorContext owns editor state.
Scene still owns scene data.
Renderer still owns rendering.
MapEditorApp owns lifetime wiring.
```

---

# 6. Dear ImGui Layer

## 6.1 Files

```text
ImGuiLayer.hpp/cpp
ImGuiRendererBridge.hpp/cpp
ImGuiUtils.hpp/cpp
```

## 6.2 Requirements

Implement:

```text
Dear ImGui context creation
docking enabled
keyboard navigation enabled
gamepad navigation optional
dark theme
font loading path
frame begin/end
Vulkan renderer backend integration
SDL3 platform backend integration
clean shutdown
```

If SDL3 + Vulkan ImGui backends require custom source handling, isolate backend-specific logic inside `ImGuiLayer`.

## 6.3 ImGui Config

Enable:

```cpp
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
```

Optional:

```cpp
io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
```

Use multi-viewports only if it works cleanly on both Windows and Linux.

## 6.4 ImGui Vulkan Integration

Renderer should provide a controlled bridge for ImGui draw data.

Implement a bridge such as:

```cpp
class ImGuiRendererBridge {
public:
    Status Init(Renderer& renderer, Window& window);
    void Shutdown();

    void NewFrame();
    void RenderDrawData(ImDrawData* drawData);
};
```

Renderer may have editor-only integration methods if needed, but they must be isolated.

---

# 7. Dockspace and Layout

## 7.1 Dockspace

Implement:

```text
fullscreen dockspace
main menu bar
persistent layout
default layout creation
```

Default layout:

```text
Top: main menu + toolbar
Left: Hierarchy
Center: Scene Viewport
Right: Inspector
Bottom: Project + Console + Stats tabs
Optional: Scene Validation / Prefab panels as tabs
```

## 7.2 Layout Persistence

Store layout:

```text
data/editor/layout.ini
```

Store editor settings:

```text
data/editor/editor_settings.toml
```

## 7.3 Required Behavior

```text
dock panels
undock panels
resize panels
restore layout on restart
reset layout from menu
```

---

# 8. Main Menu Bar

## 8.1 Files

```text
MainMenuBar.hpp/cpp
```

## 8.2 Required Menus

Implement:

```text
File
Edit
GameObject
Component
Tools
View
Window
Help
```

## 8.3 File Menu

Actions:

```text
New Scene
Open Scene
Save Scene
Save Scene As
Open Recent
Exit
```

Required behavior:

```text
New Scene creates a clean editable scene.
Open Scene uses file dialog and loads .scene.yaml.
Save Scene saves active scene path.
Save Scene As uses file dialog.
Warn on unsaved changes before destructive actions.
Open Recent tracks recent scenes.
```

## 8.4 Edit Menu

Actions:

```text
Undo
Redo
Cut
Copy
Paste
Duplicate
Delete
Select All
Deselect
Preferences
```

## 8.5 GameObject Menu

Actions:

```text
Create Empty
Create Empty Child
Create Camera
Create Directional Light
Create Point Light
Create Spot Light
Create Rink Marker
Create Puck Spawn
Create Home Goal
Create Away Goal
Create Spawn Point
Create Faceoff Spot
```

## 8.6 Component Menu

Actions:

```text
Add Component
Remove Component
Reset Component if supported
```

## 8.7 Tools Menu

Actions:

```text
Select Tool
Move Tool
Rotate Tool
Scale Tool
Hockey Rink Tool
Hockey Spawn Tool
Hockey Goal Tool
Hockey Puck Tool
Hockey Faceoff Tool
Light Tool
Validate Scene
```

## 8.8 View/Window Menu

Actions:

```text
show/hide Hierarchy
show/hide Inspector
show/hide Scene Viewport
show/hide Game Viewport
show/hide Project
show/hide Console
show/hide Stats
show/hide Scene Validation
reset layout
```

## 8.9 Help Menu

Actions:

```text
About
Controls
Project Rules
```

---

# 9. Toolbar

## 9.1 Files

```text
Toolbar.hpp/cpp
```

## 9.2 Required Toolbar Controls

Implement:

```text
select/move/rotate/scale tool buttons
local/global transform toggle
snap toggle
grid toggle
play button
simulate button
pause button
step button
save button
validate scene button
current scene name
dirty scene indicator
```

## 9.3 Play/Simulate Rules

Phase 4 does not implement gameplay.

Toolbar play/simulate controls should call scene lifecycle only:

```text
Play:
  OnRuntimeStart
  OnRuntimeStop

Simulate:
  OnSimulationStart
  OnSimulationStop
```

No physics/gameplay simulation is required.

Step button can call one fixed update if scene systems exist, but it should not require physics/gameplay.

---

# 10. Panel System

## 10.1 Base Panel

Files:

```text
Panel.hpp/cpp
PanelManager.hpp/cpp
```

Implement:

```cpp
class Panel {
public:
    explicit Panel(std::string name);
    virtual ~Panel() = default;

    const std::string& GetName() const;

    bool IsOpen() const;
    void SetOpen(bool open);

    virtual void OnUpdate(EditorContext& context, float deltaTime) {}
    virtual void OnImGui(EditorContext& context) = 0;

private:
    std::string m_Name;
    bool m_Open = true;
};
```

PanelManager:

```cpp
class PanelManager {
public:
    template<typename T, typename... Args>
    T& AddPanel(Args&&... args);

    Panel* FindPanel(const std::string& name);

    void OnUpdate(EditorContext& context, float deltaTime);
    void OnImGui(EditorContext& context);

    void SetPanelOpen(const std::string& name, bool open);
};
```

## 10.2 Required Panels

Implement:

```text
HierarchyPanel
InspectorPanel
SceneViewportPanel
GameViewportPanel
ProjectPanel
ConsolePanel
PropertiesPanel
StatsPanel
SceneValidationPanel
PrefabPanel
```

---

# 11. Hierarchy Panel

## 11.1 Files

```text
Panels/HierarchyPanel.hpp/cpp
```

## 11.2 Required Behavior

Implement:

```text
display root entities
display child entities recursively
expand/collapse tree nodes
select entity
multi-select optional
rename entity inline
right-click context menu
drag/drop reparenting
drag/drop reorder if possible
create empty child
duplicate
delete
focus selected
```

## 11.3 Context Menu

Actions:

```text
Create Empty
Create Empty Child
Duplicate
Rename
Delete
Copy
Paste Child
Unparent
Create Prefab From Entity
Validate Entity
```

## 11.4 Drag/Drop Reparenting

Rules:

```text
cannot parent entity to itself
cannot parent entity to descendant
preserve world transform by default
mark scene dirty
push undo command
```

## 11.5 Selection Sync

Hierarchy selection must sync with:

```text
InspectorPanel
SceneViewportPanel
Gizmos
PropertiesPanel
PrefabPanel
```

---

# 12. Inspector Panel

## 12.1 Files

```text
Panels/InspectorPanel.hpp/cpp
Inspector/ComponentInspector.hpp/cpp
Inspector/FieldDrawers.hpp/cpp
Inspector/ComponentAddMenu.hpp/cpp
```

## 12.2 Required Behavior

Inspector must use ECS component metadata.

Implement:

```text
show selected entity name
edit selected entity name
show active toggle
show UUID read-only
show components
edit component fields
add component
remove component
reset component where supported
copy/paste component values if feasible
multi-entity editing optional
```

## 12.3 Required Field Drawers

Implement field editing for:

```text
bool
int
float
string
vec2
vec3
vec4
enum
UUID read-only
path/string file reference
```

Use metadata:

```text
FieldType
offset
displayName
readOnly
min/max/speed
enumNames/enumValues
```

## 12.4 Component Add Menu

The Add Component menu must be generated from `ComponentRegistry`.

Rules:

```text
hide components already present unless multiple allowed
hide non-addable internal components such as ParentComponent/ChildrenComponent unless intentionally exposed
allow search/filter
add selected component
mark scene dirty
push undo command
```

## 12.5 Remove Component

Rules:

```text
cannot remove required components:
  IDComponent
  TransformComponent
  ChildrenComponent
can remove optional components
mark scene dirty
push undo command
```

---

# 13. Scene Viewport Panel

## 13.1 Files

```text
Panels/SceneViewportPanel.hpp/cpp
EditorCamera.hpp/cpp
Gizmos/TransformGizmo.hpp/cpp
Gizmos/GridGizmo.hpp/cpp
Gizmos/SelectionOutline.hpp/cpp
```

## 13.2 Required Behavior

Implement:

```text
render active scene to offscreen renderer target
display render target in ImGui
resize render target when panel size changes
editor camera navigation
entity selection from viewport
transform gizmos
grid display
selection outline or selection highlight
focus selected entity
frame selected entity
drag/drop prefab into viewport
```

## 13.3 Editor Camera Controls

Implement controls:

```text
right mouse + WASD = fly camera
right mouse + QE = up/down
mouse wheel = zoom/speed
middle mouse drag = pan
Alt + left mouse = orbit selected
F = frame selected
```

Make controls configurable later via editor settings.

## 13.4 Viewport Selection

Implement one of:

```text
CPU ray/entity bounds picking
GPU ID picking
simple approximate picking using entity bounds
```

Phase 4 complete requirement:

```text
Clicking visible selectable editor objects can select them.
Selection syncs with hierarchy and inspector.
```

## 13.5 Transform Gizmos

Use ImGuizmo if available.

Required operations:

```text
translate
rotate
scale
local/global mode
snap on/off
snap increments
undo command on manipulation complete
mark scene dirty
```

If ImGuizmo is not used, implement simple numeric inspector editing and viewport visual selection, but do not claim full gizmo support unless actual manipulation exists.

## 13.6 Grid

Implement:

```text
grid visible in viewport
grid spacing setting
grid toggle
```

Renderer debug draw can be used.

---

# 14. Game Viewport Panel

## 14.1 Files

```text
Panels/GameViewportPanel.hpp/cpp
```

## 14.2 Purpose

Game Viewport previews the scene from the primary `CameraComponent`.

## 14.3 Required Behavior

Implement:

```text
find primary camera
render scene to offscreen target using primary camera
display target in panel
show message if no primary camera
allow set selected camera as primary
resize target when panel changes
```

No gameplay simulation is required in Phase 4.

---

# 15. Project Panel

## 15.1 Files

```text
Panels/ProjectPanel.hpp/cpp
Project/ProjectBrowser.hpp/cpp
Project/FileTypeRegistry.hpp/cpp
Project/EditorAssetPreview.hpp/cpp
```

## 15.2 Required Behavior

Browse project files under:

```text
data/raw
data/raw/scenes
data/raw/prefabs
data/editor
```

Show file types:

```text
.scene.yaml
.prefab.yaml
.toml
.glsl
.spv
known image/model extensions as placeholders only
```

## 15.3 Required Actions

Implement:

```text
open scene
save scene as selected path
instantiate prefab
create folder
rename file
delete file with confirmation
reveal path in OS file explorer if easy
refresh
search/filter
```

## 15.4 Drag/Drop

Implement:

```text
drag prefab into hierarchy
drag prefab into scene viewport
drag scene file to open with confirmation
```

Do not implement final asset import/cooking in Phase 4.

Project panel can display unsupported assets but should not import them.

---

# 16. Console Panel

## 16.1 Files

```text
Panels/ConsolePanel.hpp/cpp
Logging/EditorConsoleSink.hpp/cpp
```

## 16.2 Required Behavior

Implement an editor log sink that captures messages.

Console panel supports:

```text
show logs
filter trace/info/warn/error/critical
search text
clear
copy selected log
auto-scroll
pause auto-scroll
collapse duplicate messages if feasible
```

## 16.3 Log Entry Data

Implement:

```cpp
struct EditorLogEntry {
    std::string message;
    std::string loggerName;
    std::string level;
    std::string timestamp;
    std::string sourceFile;
    int sourceLine = 0;
};
```

Source file/line optional depending on spdlog configuration.

---

# 17. Stats Panel

## 17.1 Files

```text
Panels/StatsPanel.hpp/cpp
```

## 17.2 Required Stats

Display:

```text
FPS
frame time
CPU frame ms
GPU frame ms if available
draw calls
triangles
mesh count
material count
texture count
VRAM usage if available
scene entity count
selected entity count
renderer resolution
viewport resolution
```

Use ImGui tables.

If ImPlot is included, graphs may be added:

```text
frame time history
draw call history
GPU ms history
```

---

# 18. Scene Validation Panel

## 18.1 Files

```text
Panels/SceneValidationPanel.hpp/cpp
```

## 18.2 Required Behavior

Use Phase 2 `SceneValidator`.

Show:

```text
warnings
errors
entity references
click issue to select entity
validate button
validate on scene load
validate before save if setting enabled
```

Warnings should not block save unless editor setting says so.

Errors should be clearly visible.

---

# 19. Prefab Panel

## 19.1 Files

```text
Panels/PrefabPanel.hpp/cpp
```

## 19.2 Required Behavior

Implement:

```text
show selected entity prefab status
show source prefab path
show prefab asset ID
show source entity ID
create prefab from selected entity
instantiate prefab
show overrides if available
apply override if Phase 2 supports it
revert override if Phase 2 supports it
open prefab source
```

If apply/revert override support is incomplete in ECS, expose status and creation/instancing fully, and keep apply/revert disabled with clear tooltips.

---

# 20. Selection System

## 20.1 Files

```text
Selection.hpp/cpp
```

## 20.2 API

Implement:

```cpp
class Selection {
public:
    void Select(UUID entityId);
    void Add(UUID entityId);
    void Remove(UUID entityId);
    void Toggle(UUID entityId);
    void Clear();

    bool IsSelected(UUID entityId) const;
    UUID Primary() const;
    const std::vector<UUID>& All() const;

    size_t Count() const;
};
```

## 20.3 Behavior

```text
single select
multi-select if using Ctrl/Shift
primary selection
clear on scene close
drop invalid selections after scene reload/delete
selection changed events/callbacks if useful
```

Selection syncs:

```text
Hierarchy
Inspector
Viewport
Properties
Prefab panel
```

---

# 21. Undo/Redo and Commands

## 21.1 Files

```text
EditorCommands.hpp/cpp
UndoRedo.hpp/cpp
```

## 21.2 Command Interface

Implement:

```cpp
class EditorCommand {
public:
    virtual ~EditorCommand() = default;
    virtual void Execute(EditorContext& context) = 0;
    virtual void Undo(EditorContext& context) = 0;
    virtual std::string Name() const = 0;
};
```

Undo stack:

```cpp
class UndoRedoStack {
public:
    void Execute(std::unique_ptr<EditorCommand> command, EditorContext& context);
    void Undo(EditorContext& context);
    void Redo(EditorContext& context);

    bool CanUndo() const;
    bool CanRedo() const;

    std::string UndoName() const;
    std::string RedoName() const;

    void Clear();
};
```

## 21.3 Required Commands

Implement:

```text
CreateEntityCommand
DeleteEntityCommand
DuplicateEntityCommand
RenameEntityCommand
SetActiveCommand
SetParentCommand
TransformEntityCommand
AddComponentCommand
RemoveComponentCommand
EditComponentFieldCommand
CreatePrefabCommand
InstantiatePrefabCommand
```

## 21.4 Dirty State

Commands that change scene must:

```text
mark scene dirty
record undo
update selection if needed
```

Undo/redo must preserve:

```text
UUIDs where appropriate
hierarchy
component data
selection where appropriate
```

---

# 22. Clipboard

## 22.1 Files

```text
Clipboard.hpp/cpp
```

## 22.2 Required Behavior

Implement:

```text
copy selected entity/entities
cut selected entity/entities
paste as root
paste as child of selected entity
copy component data
paste component data where compatible
```

Use YAML serialization to clipboard internally if useful.

Clipboard can be editor-internal in Phase 4.

OS clipboard integration is optional.

---

# 23. Editor Tools

## 23.1 Base Tool

Files:

```text
Tools/EditorTool.hpp/cpp
Tools/ToolManager.hpp/cpp
```

Implement:

```cpp
class EditorTool {
public:
    virtual ~EditorTool() = default;
    virtual const char* Name() const = 0;
    virtual void OnSelected(EditorContext& context) {}
    virtual void OnDeselected(EditorContext& context) {}
    virtual void OnUpdate(EditorContext& context, float deltaTime) {}
    virtual void OnViewportImGui(EditorContext& context) {}
};
```

ToolManager:

```text
register tools
select active tool
draw toolbar buttons
route viewport input to active tool
```

## 23.2 Required Generic Tools

Implement:

```text
SelectTool
MoveTool
RotateTool
ScaleTool
```

## 23.3 Hockey Tools

Implement hockey-specific placement tools:

```text
HockeyRinkTool
HockeySpawnTool
HockeyGoalTool
HockeyPuckTool
HockeyFaceoffTool
HockeyCameraRigTool
LightTool
```

These tools create scene marker entities/components only.

They do not implement hockey gameplay.

## 23.4 Hockey Tool Requirements

### HockeyRinkTool

Creates:

```text
Rink root entity
RinkComponent
PlayAreaComponent
default board/glass visual marker if render components exist
```

### HockeySpawnTool

Creates spawn markers for:

```text
Home Spawn 0
Home Spawn 1
Home Spawn 2
Home Spawn 3
Away Spawn 0
Away Spawn 1
Away Spawn 2
Away Spawn 3
```

Each has:

```text
SpawnPointComponent with FaceoffSpawn=false
TeamComponent
TransformComponent
```

### HockeyGoalTool

Creates:

```text
Home Goal
Away Goal
```

Each has:

```text
GoalComponent
TransformComponent
optional MeshRendererComponent with BuiltIn.GoalNet
```

### HockeyPuckTool

Creates:

```text
Puck Spawn
```

With:

```text
PuckComponent
TransformComponent
optional MeshRendererComponent with BuiltIn.PuckCylinder/BuiltIn.PuckBlack
```

### HockeyFaceoffTool

Creates:

```text
8 Neutral Faceoff Spawn markers
8 Home Penalty Faceoff Spawn markers
8 Away Penalty Faceoff Spawn markers
```

With:

```text
SpawnPointComponent with FaceoffSpawn=true
TransformComponent
```

### HockeyCameraRigTool

Creates:

```text
Gameplay Camera Rig
```

With:

```text
CameraRigMarkerComponent
CameraComponent if appropriate
TransformComponent
```

### LightTool

Creates:

```text
Directional Light
Point Light
Spot Light
```

With:

```text
LightComponent
TransformComponent
```

---

# 24. Editor Settings

## 24.1 Files

```text
EditorSettings.hpp/cpp
data/editor/editor_settings.toml
data/editor/themes/dark.toml
```

## 24.2 Settings Struct

Implement:

```cpp
struct EditorSettings {
    bool autosaveEnabled = true;
    int autosaveIntervalSeconds = 120;

    bool validateBeforeSave = true;
    bool validateAfterLoad = true;

    bool showGrid = true;
    float gridSpacing = 1.0f;

    bool snapEnabled = false;
    float moveSnap = 0.5f;
    float rotateSnapDegrees = 15.0f;
    float scaleSnap = 0.1f;

    float cameraMoveSpeed = 10.0f;
    float cameraFastMultiplier = 4.0f;
    float cameraMouseSensitivity = 0.15f;

    bool restoreLastScene = true;
    std::vector<std::filesystem::path> recentScenes;
};
```

## 24.3 Required Behavior

```text
load settings on editor start
save settings on editor shutdown
save settings when changed
recent scenes persist
snap settings persist
grid settings persist
camera settings persist
```

---

# 25. Autosave and Unsaved Changes

## 25.1 Dirty Flag

Scene dirty flag must be set when:

```text
entity created
entity deleted
entity duplicated
entity renamed
entity reparented
entity transform changed
component added
component removed
component field changed
prefab created/instantiated
```

## 25.2 Unsaved Changes Prompt

Prompt on:

```text
new scene
open scene
exit editor
```

Options:

```text
Save
Discard
Cancel
```

## 25.3 Autosave

If enabled:

```text
save autosave copy every autosaveIntervalSeconds
only when dirty
do not overwrite user scene if autosave path is separate
```

Autosave path example:

```text
data/temp/autosaves/<scene_name>.autosave.scene.yaml
```

Provide menu option:

```text
Open Autosave Folder
```

---

# 26. Scene Save/Load Workflow

## 26.1 New Scene

Behavior:

```text
create empty Scene in Edit mode
clear selection
clear undo stack
clear activeScenePath or set untitled
mark dirty false
```

Optionally create default:

```text
Editor Camera entity
Directional Light
```

## 26.2 Open Scene

Behavior:

```text
prompt unsaved changes
use file dialog
load .scene.yaml
validate if enabled
set activeScenePath
clear undo stack
clear selection or preserve if entity UUID still exists
mark dirty false
add to recent scenes
```

## 26.3 Save Scene

Behavior:

```text
if activeScenePath exists:
  validate if enabled
  serialize scene
  mark dirty false
else:
  Save Scene As
```

## 26.4 Save Scene As

Behavior:

```text
use file dialog
ensure .scene.yaml extension
validate if enabled
serialize scene
set activeScenePath
mark dirty false
add to recent scenes
```

---

# 27. Prefab Workflow

## 27.1 Create Prefab

From selected entity:

```text
right click entity -> Create Prefab
or Prefab panel -> Create Prefab
choose path under data/raw/prefabs
save prefab hierarchy
retain scene entity unchanged
show PrefabComponent if ECS supports source link
mark scene dirty only if scene entity is changed
```

## 27.2 Instantiate Prefab

From project panel or prefab panel:

```text
load .prefab.yaml
instantiate into active scene
select instantiated root
mark scene dirty
push undo command
```

## 27.3 Drag/Drop Prefab

Dragging `.prefab.yaml` into:

```text
Hierarchy:
  instantiate as root or child of hovered entity

Scene Viewport:
  instantiate at picked world position
```

If world picking is not available, instantiate at origin and select it.

---

# 28. Editor Visual Feedback

Implement:

```text
selected entity highlight
hovered entity highlight if picking supports hover
grid
gizmo axes
camera frustum icon/visual if possible
light icon/visual if possible
spawn marker debug shape
goal marker debug shape
faceoff marker debug shape
puck marker debug shape
```

Use renderer debug draw and built-in meshes/materials.

Do not depend on final art assets.

---

# 29. Keyboard Shortcuts

Implement shortcuts:

```text
Ctrl+N New Scene
Ctrl+O Open Scene
Ctrl+S Save Scene
Ctrl+Shift+S Save Scene As
Ctrl+Z Undo
Ctrl+Y Redo
Ctrl+X Cut
Ctrl+C Copy
Ctrl+V Paste
Ctrl+D Duplicate
Delete Delete Entity
F Frame Selected
Q Select Tool
W Move Tool
E Rotate Tool
R Scale Tool
Ctrl+P Play/Stop if desired
```

Shortcuts should not trigger while typing in text fields.

Use ImGui input state and editor context.

---

# 30. Linux/Windows Compatibility

Phase 4 must work on Windows and Linux.

Rules:

```text
Use std::filesystem for paths.
Do not hardcode path separators.
Do not use Windows-only file dialogs directly.
Use nativefiledialog-extended or an abstraction.
Do not assume fonts/icons exist in OS-specific locations.
Do not assume external editor tools exist.
Do not link editor into server.
```

File dialogs must work on both platforms or gracefully fall back to typed path input.

If native dialogs fail on Linux due to missing desktop portal dependencies, editor must still be usable through internal path text fields.

---

# 31. Tests

Create:

```text
HockeyEditorTests
```

Use simple test harness.

## 31.1 EditorSettingsTests

Test:

```text
default settings valid
load settings TOML
save settings TOML
recent scenes persist
snap settings persist
camera settings persist
```

## 31.2 SelectionTests

Test:

```text
select entity
add entity
remove entity
toggle entity
clear selection
primary selection
invalid selection removed after scene deletion
```

## 31.3 UndoRedoTests

Test:

```text
execute command
undo command
redo command
clear stack
create entity undo/redo
rename entity undo/redo
transform undo/redo
add component undo/redo
remove component undo/redo
```

## 31.4 InspectorFieldTests

Test:

```text
bool field edit
int field edit
float field edit
string field edit
vec2 field edit
vec3 field edit
vec4 field edit
enum field edit
read-only field does not edit
component metadata field lookup works
```

## 31.5 SceneCommandTests

Test:

```text
new scene command
open scene command if file exists
save scene command
delete entity command
duplicate entity command
parent command
dirty flag changes
```

## 31.6 ProjectBrowserTests

Test:

```text
scan data/raw
detect scenes
detect prefabs
filter by extension
search files
create folder in temp test path
```

## 31.7 HockeyToolTests

Test:

```text
rink tool creates RinkComponent and PlayAreaComponent
spawn tool creates valid SpawnPointComponent
goal tool creates GoalComponent
puck tool creates PuckComponent
faceoff tool creates neutral/Home/Away faceoff SpawnPointComponent pools
camera rig tool creates CameraRigMarkerComponent
light tool creates LightComponent
scene validation passes for generated marker set where expected
```

## 31.8 Editor Smoke Test

If GUI environment exists:

```text
launch map editor
create ImGui context
create dockspace
render one editor frame
shutdown cleanly
```

Mark GUI smoke tests optional if CI/headless environment cannot run them.

---

# 32. Implementation Order for AI Agents

Implement in this exact order.

## Step 1 — Dependency and Target Setup

```text
1. Add Dear ImGui dependency.
2. Add nativefiledialog-extended dependency.
3. Add ImGuizmo dependency or local source if approved.
4. Create libs/editor folder.
5. Create libs/editor/CMakeLists.txt.
6. Create hockey_editor target.
7. Create apps/editor_tests target.
8. Update root CMakeLists.txt.
9. Link HockeyMapEditor to hockey_editor.
10. Confirm HockeyDedicatedServer does not link hockey_editor.
11. Configure/build before complex code.
```

## Step 2 — Editor Context and Settings

```text
1. Add EditorContext.
2. Add EditorSettings.
3. Add settings load/save.
4. Add editor data folders.
5. Add EditorSettingsTests.
```

## Step 3 — ImGui Layer

```text
1. Add ImGuiLayer.
2. Add ImGuiRendererBridge.
3. Initialize ImGui with SDL3 + Vulkan renderer.
4. Enable docking.
5. Apply dark theme.
6. Render empty dockspace frame.
7. Shutdown cleanly.
```

## Step 4 — Dockspace, Menu, Toolbar

```text
1. Add Dockspace.
2. Add MainMenuBar.
3. Add Toolbar.
4. Implement File/Edit/GameObject/Component/Tools/View/Window/Help menus.
5. Implement toolbar buttons.
6. Implement layout persistence/reset.
```

## Step 5 — Panel System

```text
1. Add Panel base.
2. Add PanelManager.
3. Register all panels.
4. Add show/hide panel menu.
5. Add empty versions of all panels.
```

## Step 6 — Selection and Hierarchy

```text
1. Add Selection.
2. Add SelectionTests.
3. Implement HierarchyPanel.
4. Implement tree display.
5. Implement select/rename/delete/duplicate.
6. Implement drag/drop reparent.
7. Sync selection with context.
```

## Step 7 — Inspector

```text
1. Add ComponentInspector.
2. Add FieldDrawers.
3. Add ComponentAddMenu.
4. Draw selected entity header.
5. Draw required components.
6. Draw editable fields from metadata.
7. Add/remove optional components.
8. Add inspector field tests.
```

## Step 8 — Undo/Redo and Clipboard

```text
1. Add EditorCommand interface.
2. Add UndoRedoStack.
3. Implement required scene commands.
4. Wire commands into hierarchy/inspector/menu.
5. Add Clipboard.
6. Add undo/redo tests.
```

## Step 9 — Scene Viewport

```text
1. Add SceneViewportPanel.
2. Add EditorCamera.
3. Create offscreen render target.
4. Display render target in ImGui.
5. Implement camera controls.
6. Implement viewport resize.
7. Implement grid.
8. Implement selection highlight.
9. Implement entity picking.
10. Implement transform gizmos.
```

## Step 10 — Game Viewport

```text
1. Add GameViewportPanel.
2. Find primary CameraComponent.
3. Render scene through primary camera.
4. Show message if no primary camera.
5. Add set selected camera primary action.
```

## Step 11 — Project, Console, Stats, Validation, Prefab Panels

```text
1. Add ProjectPanel.
2. Add ProjectBrowser.
3. Add FileTypeRegistry.
4. Add ConsolePanel and EditorConsoleSink.
5. Add StatsPanel.
6. Add SceneValidationPanel.
7. Add PrefabPanel.
8. Wire panel menus/actions.
```

## Step 12 — Editor Tools

```text
1. Add EditorTool base.
2. Add ToolManager.
3. Add Select/Move/Rotate/Scale tools.
4. Add hockey placement tools.
5. Add LightTool.
6. Add hockey tool tests.
7. Wire tools into toolbar/menu/viewport.
```

## Step 13 — Scene and Prefab Workflows

```text
1. Implement New Scene.
2. Implement Open Scene.
3. Implement Save Scene.
4. Implement Save Scene As.
5. Implement recent scenes.
6. Implement unsaved changes prompt.
7. Implement autosave.
8. Implement create prefab.
9. Implement instantiate prefab.
10. Implement prefab drag/drop.
```

## Step 14 — Final Integration

```text
1. Update HockeyMapEditor app.
2. Ensure editor initializes renderer and ImGui.
3. Load startup scene.
4. Render editor UI and viewport.
5. Handle shutdown order.
6. Ensure client still builds/runs.
7. Ensure server still builds/runs headless.
```

## Step 15 — Final Verification

```text
1. Build Debug Windows.
2. Build Release Windows.
3. Build Debug Linux.
4. Build Release Linux.
5. Run HockeyCoreTests.
6. Run HockeyECSTests.
7. Run HockeyRendererTests.
8. Run HockeyEditorTests.
9. Run HockeyMapEditor.
10. Create scene.
11. Add entities/components.
12. Save scene.
13. Reload scene.
14. Create prefab.
15. Instantiate prefab.
16. Validate hockey scene.
17. Confirm server does not link editor/renderer.
```

---

# 33. Verification Commands

## Windows

```powershell
.\\scripts\\windows\\configure_debug.ps1
.\\scripts\\windows\\build_debug.ps1
.\\build\\windows-debug\\apps\\core_tests\\HockeyCoreTests.exe --root .
.\\build\\windows-debug\\apps\\ecs_tests\\HockeyECSTests.exe --root .
.\\build\\windows-debug\\apps\\renderer_tests\\HockeyRendererTests.exe --root .
.\\build\\windows-debug\\apps\\editor_tests\\HockeyEditorTests.exe --root .
.\\scripts\\windows\\run_editor.ps1
.\\scripts\\windows\\run_client.ps1
.\\scripts\\windows\\run_server.ps1
```

## Linux

```bash
./scripts/linux/configure_debug.sh
./scripts/linux/build_debug.sh
./build/linux-debug/apps/core_tests/HockeyCoreTests --root .
./build/linux-debug/apps/ecs_tests/HockeyECSTests --root .
./build/linux-debug/apps/renderer_tests/HockeyRendererTests --root .
./build/linux-debug/apps/editor_tests/HockeyEditorTests --root .
./scripts/linux/run_editor.sh
./scripts/linux/run_client.sh
./scripts/linux/run_server.sh
```

---

# 34. Phase 4 Definition of Done

Phase 4 is complete only when all of this is true:

```text
editor dependencies added through vcpkg or approved local source
hockey_editor builds
HockeyEditorTests builds
HockeyEditorTests pass
HockeyMapEditor links hockey_editor
HockeyGameClient does not link hockey_editor
HockeyDedicatedServer does not link hockey_editor
HockeyDedicatedServer does not link ImGui
ImGui initializes
docking layout works
layout persists
main menu works
toolbar works
panel system works
hierarchy panel works
inspector panel works
component metadata editing works
add component menu works
remove component works
scene viewport renders offscreen target
editor camera controls work
viewport selection works
transform gizmos work
grid works
game viewport works
project panel browses data/raw
scene open/save/save-as works
recent scenes work
unsaved changes prompt works
autosave works
console panel captures logs
stats panel displays renderer/editor stats
scene validation panel works
prefab panel works
create prefab works
instantiate prefab works
prefab drag/drop works
selection system works
undo/redo works
clipboard works
generic tools work
hockey placement tools work
editor settings persist
Windows editor works
Linux editor works
client still runs
server still runs headless
no physics simulation exists
no networking code exists
no hockey gameplay simulation exists
no final asset pipeline exists
```

---

# 35. AI Completion Instruction

When Phase 4 is complete, AI agent should report:

```text
Phase 4 complete.

Implemented:
- hockey_editor
- HockeyEditorTests
- Dear ImGui docking editor layer
- editor context/settings/theme
- main menu bar
- toolbar
- panel manager
- hierarchy panel
- inspector panel
- scene viewport panel
- game viewport panel
- project panel
- console panel
- stats panel
- scene validation panel
- prefab panel
- selection system
- editor camera
- transform gizmos
- grid/selection visuals
- component metadata field editing
- add/remove component workflow
- undo/redo command system
- clipboard
- project browser
- scene new/open/save/save-as
- recent scenes
- unsaved changes prompt
- autosave
- prefab create/instantiate/drag-drop
- hockey-specific placement tools
- editor settings persistence
- map editor app integration

Verified:
- editor tests
- map editor launches
- docking works
- scene edit/save/load works
- prefab workflow works
- hockey tools create valid marker entities
- server remains headless
- client still runs
- no future-phase systems were implemented
```

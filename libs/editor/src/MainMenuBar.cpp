#include "Hockey/Editor/MainMenuBar.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <imgui.h>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorApp.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Panel.hpp"
#include "Hockey/Editor/PanelManager.hpp"

namespace Hockey {

void MainMenuBar::Draw(EditorContext& ctx, EditorApp& app) {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            DrawFileMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            DrawEditMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("GameObject")) {
            DrawGameObjectMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Component")) {
            DrawComponentMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            DrawToolsMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            DrawViewMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            DrawWindowMenu(ctx, app);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            DrawHelpMenu(ctx, app);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    DrawHelpPopups();
}

void MainMenuBar::DrawFileMenu(EditorContext& ctx, EditorApp& app) {
    if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        app.NewScene();
    }
    if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
        app.OpenScene();
    }
    if (ImGui::BeginMenu("Open Recent", !ctx.settings.recentScenes.empty())) {
        for (const std::filesystem::path& recent : ctx.settings.recentScenes) {
            if (ImGui::MenuItem(recent.string().c_str())) {
                app.OpenScene(recent);
            }
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        app.SaveScene();
    }
    if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
        app.SaveSceneAs();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Import Asset...")) {
        app.ImportAsset();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Open Autosave Folder")) {
        app.OpenAutosaveFolder();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Exit", "Alt+F4")) {
        app.RequestQuit();
    }
}

void MainMenuBar::DrawEditMenu(EditorContext& ctx, EditorApp& app) {
    const std::string undoName = ctx.undoRedo.UndoName();
    const std::string redoName = ctx.undoRedo.RedoName();
    const std::string undoLabel = undoName.empty() ? "Undo" : ("Undo " + undoName);
    const std::string redoLabel = redoName.empty() ? "Redo" : ("Redo " + redoName);

    if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, ctx.undoRedo.CanUndo())) {
        app.Undo();
    }
    if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, ctx.undoRedo.CanRedo())) {
        app.Redo();
    }
    ImGui::Separator();

    const bool hasSelection = !ctx.selection.Empty();
    const bool hasPrimary = ctx.selection.Primary().IsValid();
    const bool canPaste = ctx.clipboard.HasEntities();
    if (ImGui::MenuItem("Cut", "Ctrl+X", false, hasSelection)) {
        app.CutSelection();
    }
    if (ImGui::MenuItem("Copy", "Ctrl+C", false, hasSelection)) {
        app.CopySelection();
    }
    if (ImGui::MenuItem("Paste", "Ctrl+V", false, canPaste)) {
        app.PasteFromClipboard(/*asChildOfSelection=*/false);
    }
    if (ImGui::MenuItem("Paste As Child", nullptr, false, canPaste && hasPrimary)) {
        app.PasteFromClipboard(/*asChildOfSelection=*/true);
    }
    if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, hasSelection)) {
        app.DuplicateSelection();
    }
    if (ImGui::MenuItem("Delete", "Del", false, hasSelection)) {
        app.DeleteSelection();
    }
    ImGui::Separator();
    const bool hasScene = ctx.activeScene != nullptr;
    if (ImGui::MenuItem("Select All", "Ctrl+A", false, hasScene)) {
        app.SelectAllEntities();
    }
    if (ImGui::MenuItem("Deselect", "Esc", false, hasSelection)) {
        ctx.selection.Clear();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Preferences...")) {
        app.OpenPreferences();
    }
}

void MainMenuBar::DrawGameObjectMenu(EditorContext& ctx, EditorApp& /*app*/) {
    const bool hasScene = ctx.activeScene != nullptr;
    if (ImGui::MenuItem("Create Empty", nullptr, false, hasScene)) {
        ctx.undoRedo.Execute(EditorCommands::CreateEntity("GameObject"), ctx);
    }
    if (ImGui::MenuItem("Create Empty Child", nullptr, false, hasScene && ctx.selection.Primary().IsValid())) {
        ctx.undoRedo.Execute(EditorCommands::CreateEntity("GameObject", ctx.selection.Primary()), ctx);
    }
    if (ImGui::MenuItem("Create Camera", nullptr, false, hasScene)) {
        ctx.undoRedo.Execute(EditorCommands::SpawnEntities("Create Camera",
                                                           [](Scene& scene) {
                                                               Entity camera = scene.CreateEntity("Camera");
                                                               camera.AddComponent<CameraComponent>();
                                                               return std::vector<UUID>{camera.GetUUID()};
                                                           }),
                             ctx);
    }
    ImGui::Separator();

    // Light/create tools (Category "Create").
    for (const std::unique_ptr<EditorTool>& tool : ctx.toolManager.Tools()) {
        if (std::string(tool->Category()) != "Create") {
            continue;
        }
        if (ImGui::MenuItem(tool->Name(), nullptr, false, hasScene)) {
            ctx.toolManager.Activate(tool->Name(), ctx);
        }
    }
    ImGui::Separator();

    // Hockey marker tools (Category "Hockey").
    for (const std::unique_ptr<EditorTool>& tool : ctx.toolManager.Tools()) {
        if (std::string(tool->Category()) != "Hockey") {
            continue;
        }
        if (ImGui::MenuItem(tool->Name(), nullptr, false, hasScene)) {
            ctx.toolManager.Activate(tool->Name(), ctx);
        }
    }
}

void MainMenuBar::DrawComponentMenu(EditorContext& ctx, EditorApp& /*app*/) {
    const UUID primary = ctx.selection.Primary();
    if (ctx.activeScene == nullptr || !primary.IsValid()) {
        ImGui::TextDisabled("Select an entity");
        return;
    }
    Entity entity = ctx.activeScene->FindEntityByUUID(primary);
    if (!ctx.activeScene->IsValid(entity)) {
        ImGui::TextDisabled("Select an entity");
        return;
    }

    if (ImGui::BeginMenu("Add")) {
        for (const ComponentMetadata& meta : ComponentRegistry::Get().All()) {
            if (!meta.addable) {
                continue;
            }
            const bool present = meta.has && meta.has(entity);
            if (ImGui::MenuItem(meta.displayName.c_str(), nullptr, false, !present) && meta.add) {
                ctx.undoRedo.Execute(EditorCommands::AddComponent(primary, meta.name), ctx);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Remove")) {
        bool anyRemovable = false;
        for (const ComponentMetadata& meta : ComponentRegistry::Get().All()) {
            if (!meta.removable || !(meta.has && meta.has(entity))) {
                continue;
            }
            anyRemovable = true;
            if (ImGui::MenuItem(meta.displayName.c_str())) {
                ctx.undoRedo.Execute(EditorCommands::RemoveComponent(*ctx.activeScene, primary, meta.name), ctx);
            }
        }
        if (!anyRemovable) {
            ImGui::TextDisabled("No removable components");
        }
        ImGui::EndMenu();
    }
}

void MainMenuBar::DrawToolsMenu(EditorContext& ctx, EditorApp& app) {
    const bool hasScene = ctx.activeScene != nullptr;

    // Transform tools: checkmarked to show the active tool.
    for (const std::unique_ptr<EditorTool>& tool : ctx.toolManager.Tools()) {
        if (std::string(tool->Category()) != "Transform") {
            continue;
        }
        const bool active = ctx.toolManager.IsActive(tool.get());
        if (ImGui::MenuItem(tool->Name(), tool->Shortcut(), active)) {
            ctx.toolManager.Activate(tool->Name(), ctx);
        }
    }

    ImGui::Separator();
    if (ImGui::BeginMenu("Hockey", hasScene)) {
        for (const std::unique_ptr<EditorTool>& tool : ctx.toolManager.Tools()) {
            if (std::string(tool->Category()) != "Hockey") {
                continue;
            }
            if (ImGui::MenuItem(tool->Name())) {
                ctx.toolManager.Activate(tool->Name(), ctx);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Validate Scene")) {
        app.ValidateActiveScene();
    }
}

void MainMenuBar::DrawViewMenu(EditorContext& /*ctx*/, EditorApp& app) {
    if (ImGui::MenuItem("Reset Layout")) {
        app.ResetLayout();
    }
}

void MainMenuBar::DrawWindowMenu(EditorContext& ctx, EditorApp& /*app*/) {
    for (const std::unique_ptr<Panel>& panel : ctx.panelManager.Panels()) {
        bool open = panel->IsOpen();
        if (ImGui::MenuItem(panel->GetName().c_str(), nullptr, &open)) {
            panel->SetOpen(open);
        }
    }
}

void MainMenuBar::DrawHelpMenu(EditorContext& /*ctx*/, EditorApp& /*app*/) {
    if (ImGui::MenuItem("About")) {
        m_OpenAbout = true;
    }
    if (ImGui::MenuItem("Controls")) {
        m_OpenControls = true;
    }
    if (ImGui::MenuItem("Project Rules")) {
        m_OpenProjectRules = true;
    }
}

void MainMenuBar::DrawHelpPopups() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center = viewport->GetCenter();

    if (m_OpenAbout) {
        ImGui::OpenPopup("About##editor");
        m_OpenAbout = false;
    }
    if (m_OpenControls) {
        ImGui::OpenPopup("Controls##editor");
        m_OpenControls = false;
    }
    if (m_OpenProjectRules) {
        ImGui::OpenPopup("Project Rules##editor");
        m_OpenProjectRules = false;
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("About##editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", EditorVersionString());
        ImGui::Separator();
        ImGui::TextUnformatted("Unity-style map editor for the Hockey engine.");
        ImGui::TextUnformatted("Vulkan renderer | SDL3 platform | Dear ImGui UI.");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Controls##editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Scene file:");
        ImGui::BulletText("Ctrl+N New   Ctrl+O Open   Ctrl+S Save   Ctrl+Shift+S Save As");
        ImGui::TextUnformatted("Edit:");
        ImGui::BulletText("Ctrl+Z Undo   Ctrl+Y Redo   Ctrl+X/C/V Cut/Copy/Paste");
        ImGui::BulletText("Ctrl+D Duplicate   Del Delete");
        ImGui::TextUnformatted("Tools:");
        ImGui::BulletText("Q Select   W Move   E Rotate   R Scale");
        ImGui::TextUnformatted("Viewport camera:");
        ImGui::BulletText("Right-mouse + WASD fly, QE down/up, wheel speed");
        ImGui::BulletText("Middle-mouse pan, Alt+Left orbit, F frame selected");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Project Rules##editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Architecture is layered and phase-based:");
        ImGui::BulletText("core -> ecs -> renderer -> editor");
        ImGui::BulletText("editor never links gameplay/networking/physics");
        ImGui::BulletText("dedicated server stays headless (no renderer/ImGui)");
        ImGui::Spacing();
        if (ImGui::Button("Close", ImVec2(120.0f, 0.0f))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace Hockey

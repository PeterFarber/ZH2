#include "Test.hpp"

#include <filesystem>
#include <system_error>

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/PrefabSerializer.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/SceneWorkflow.hpp"

using namespace Hockey;

namespace {

std::filesystem::path MakeTempDir(const char* sub) {
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "hockey_scene_workflow" / sub;
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

struct WorkflowFixture {
    Scene scene{"Fixture"};
    EditorContext context;
    SceneWorkflow workflow;
    WorkflowFixture() {
        context.activeScene = &scene;
    }
};

} // namespace

void RunSceneWorkflowTests() {
    HockeyTest::BeginSuite("SceneWorkflowTests");
    ComponentRegistry::Get().RegisterPhase2Components();

    // --- EnsureSceneExtension normalizes the suffix --------------------------
    {
        HK_CHECK_MSG(SceneWorkflow::EnsureSceneExtension("foo").filename() == "foo.scene.yaml",
                     "bare name gains .scene.yaml");
        HK_CHECK_MSG(SceneWorkflow::EnsureSceneExtension("foo.scene.yaml").filename() == "foo.scene.yaml",
                     "existing suffix preserved");
        HK_CHECK_MSG(SceneWorkflow::EnsureSceneExtension("foo.yaml").filename() == "foo.scene.yaml",
                     "plain yaml normalized");
    }

    // --- New Scene resets editor state ---------------------------------------
    {
        WorkflowFixture fix;
        fix.scene.CreateEntity("Old");
        fix.context.activeScenePath = "/tmp/old.scene.yaml";
        fix.context.MarkDirty(true);
        fix.context.selection.Select(fix.scene.GetRootEntities().front().GetUUID());

        fix.workflow.NewScene(fix.context);

        HK_CHECK_MSG(fix.scene.GetName() == "Untitled Scene", "new scene renamed");
        HK_CHECK_MSG(!fix.scene.FindEntityByName("Old"), "new scene drops old entities");
        HK_CHECK_MSG(fix.scene.FindEntityByName("Directional Light"), "new scene seeds default light");
        HK_CHECK_MSG(fix.context.activeScenePath.empty(), "new scene clears path");
        HK_CHECK_MSG(!fix.context.sceneDirty, "new scene is clean");
        HK_CHECK_MSG(fix.context.selection.Empty(), "new scene clears selection");
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo(), "new scene clears undo stack");
    }

    // --- Save fails without a path; Save As writes + records recents ---------
    {
        WorkflowFixture fix;
        const std::filesystem::path dir = MakeTempDir("saveas");
        fix.scene.SetName("MyLevel");
        fix.scene.CreateEntity("Alpha");
        fix.scene.CreateEntity("Beta");
        fix.context.MarkDirty(true);

        HK_CHECK_MSG(!fix.workflow.SaveScene(fix.context), "Save fails when no active path");

        const std::filesystem::path target = dir / "level"; // no extension on purpose
        const Status saved = fix.workflow.SaveSceneAs(fix.context, target);
        HK_CHECK_MSG(static_cast<bool>(saved), "Save As succeeds");
        const std::filesystem::path expected = dir / "level.scene.yaml";
        HK_CHECK_MSG(std::filesystem::exists(expected), "Save As wrote .scene.yaml");
        HK_CHECK_MSG(fix.context.activeScenePath == expected, "Save As sets active path");
        HK_CHECK_MSG(!fix.context.sceneDirty, "Save As clears dirty");
        HK_CHECK_MSG(!fix.context.settings.recentScenes.empty() &&
                         fix.context.settings.recentScenes.front() == expected.lexically_normal(),
                     "Save As records recent scene");

        // A subsequent plain Save now targets the active path.
        fix.context.MarkDirty(true);
        HK_CHECK_MSG(static_cast<bool>(fix.workflow.SaveScene(fix.context)), "Save succeeds with active path");
        HK_CHECK_MSG(!fix.context.sceneDirty, "Save clears dirty");

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // --- Save then Open round-trips entities ---------------------------------
    {
        WorkflowFixture fix;
        const std::filesystem::path dir = MakeTempDir("roundtrip");
        const std::filesystem::path file = dir / "round.scene.yaml";
        fix.scene.SetName("RoundTrip");
        fix.scene.CreateEntity("One");
        fix.scene.CreateEntity("Two");
        fix.context.MarkDirty(true);
        HK_CHECK_MSG(static_cast<bool>(fix.workflow.SaveSceneAs(fix.context, file)), "round-trip save");

        fix.workflow.NewScene(fix.context, /*createDefaults=*/false);
        HK_CHECK_EQ(fix.scene.EntityCount(), static_cast<std::size_t>(0));

        const Status opened = fix.workflow.OpenScene(fix.context, file);
        HK_CHECK_MSG(static_cast<bool>(opened), "Open succeeds");
        HK_CHECK_MSG(fix.scene.GetName() == "RoundTrip", "Open restores scene name");
        HK_CHECK_MSG(fix.scene.FindEntityByName("One") && fix.scene.FindEntityByName("Two"), "Open restores entities");
        HK_CHECK_MSG(fix.context.activeScenePath == file, "Open sets active path");
        HK_CHECK_MSG(!fix.context.sceneDirty, "Open is clean");
        HK_CHECK_MSG(!fix.context.undoRedo.CanUndo(), "Open clears undo stack");

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    // --- Opening a missing file fails and leaves the scene intact ------------
    {
        WorkflowFixture fix;
        fix.scene.CreateEntity("Keep");
        const Status opened = fix.workflow.OpenScene(fix.context, "/no/such/scene.scene.yaml");
        HK_CHECK_MSG(!opened, "Open of missing file fails");
        HK_CHECK_MSG(fix.scene.FindEntityByName("Keep"), "failed open leaves scene unchanged");
    }

    // --- Autosave writes a copy only when enabled, due, and dirty ------------
    {
        WorkflowFixture fix;
        fix.scene.SetName("AutoLevel");
        fix.scene.CreateEntity("Body");
        fix.context.settings.autosaveEnabled = true;
        fix.context.settings.autosaveIntervalSeconds = 0; // due immediately

        const std::filesystem::path autosavePath = SceneWorkflow::AutosavePathFor(fix.scene);
        std::error_code ec;
        std::filesystem::remove(autosavePath, ec);

        // Not dirty: no file produced.
        fix.context.MarkDirty(false);
        fix.workflow.TickAutosave(fix.context, 1.0f);
        HK_CHECK_MSG(!std::filesystem::exists(autosavePath), "autosave skipped when clean");

        // Dirty: a copy is written, but the dirty flag is NOT cleared.
        fix.context.MarkDirty(true);
        fix.workflow.TickAutosave(fix.context, 1.0f);
        HK_CHECK_MSG(std::filesystem::exists(autosavePath), "autosave writes when dirty");
        HK_CHECK_MSG(fix.context.sceneDirty, "autosave keeps scene dirty");

        std::filesystem::remove(autosavePath, ec);
    }

    // --- InstantiatePrefab command: instance + undo/redo ---------------------
    {
        WorkflowFixture fix;
        const std::filesystem::path dir = MakeTempDir("prefab");
        const std::filesystem::path prefabPath = dir / "Widget.prefab.yaml";

        Entity source = fix.scene.CreateEntity("Widget");
        source.AddComponent<MeshRendererComponent>().meshAsset = 111u;
        Entity child = fix.scene.CreateEntity("WidgetChild");
        fix.scene.SetParent(child, source);
        HK_CHECK_MSG(static_cast<bool>(PrefabSerializer::Save(fix.scene, source, prefabPath)), "prefab saved");

        const std::size_t before = fix.scene.EntityCount();
        fix.context.undoRedo.Execute(EditorCommands::InstantiatePrefab(prefabPath), fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 2); // root + child, fresh UUIDs
        HK_CHECK_MSG(fix.context.selection.Primary().IsValid(), "instantiate selects new root");
        HK_CHECK_MSG(fix.context.sceneDirty, "instantiate marks dirty");

        fix.context.undoRedo.Undo(fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), before);

        fix.context.undoRedo.Redo(fix.context);
        HK_CHECK_EQ(fix.scene.EntityCount(), before + 2);

        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }
}

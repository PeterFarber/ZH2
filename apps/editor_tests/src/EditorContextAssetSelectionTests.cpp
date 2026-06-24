#include "Test.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Editor/EditorContext.hpp"

using namespace Hockey;

void RunEditorContextAssetSelectionTests() {
    HockeyTest::BeginSuite("EditorContextAssetSelectionTests");

    Scene scene("AssetSelection");
    Entity entity = scene.CreateEntity("Player");

    EditorContext context;
    context.activeScene = &scene;
    context.selection.Select(entity.GetUUID());

    context.SelectAsset(AssetID{42});
    HK_CHECK_MSG(context.SelectedAsset() == AssetID{42}, "asset selection stores cooked asset id");
    HK_CHECK_MSG(!context.selection.Primary().IsValid(), "selecting an asset clears entity selection");

    context.selection.Select(entity.GetUUID());
    context.SyncAssetSelectionWithEntitySelection();
    HK_CHECK_MSG(!context.SelectedAsset().IsValid(), "selecting an entity clears stale asset selection");

    context.SelectAsset(AssetID{77});
    context.ClearAssetSelection();
    HK_CHECK_MSG(!context.SelectedAsset().IsValid(), "asset selection can be cleared explicitly");

    // --- hierarchy visibility state is editor-only and recursive ------------
    Entity child = scene.CreateEntity("Child");
    Entity sibling = scene.CreateEntity("Sibling");
    scene.SetParent(child, entity, false);
    context.SetSceneHidden(scene, entity.GetUUID(), true, true);
    HK_CHECK_MSG(context.IsSceneHidden(entity.GetUUID()), "visibility toggle hides parent");
    HK_CHECK_MSG(context.IsSceneHidden(child.GetUUID()), "visibility toggle includes descendants");
    context.SetSceneHidden(scene, entity.GetUUID(), false, false);
    HK_CHECK_MSG(!context.IsSceneHidden(entity.GetUUID()), "Alt-style visibility toggle affects only parent");
    HK_CHECK_MSG(context.IsSceneHidden(child.GetUUID()), "child visibility state is preserved by single toggle");

    context.selection.Select(entity.GetUUID());
    context.selection.Add(child.GetUUID());
    context.ToggleSelectedVisibility(scene);
    HK_CHECK_MSG(context.IsSceneHidden(entity.GetUUID()) && context.IsSceneHidden(child.GetUUID()),
                 "H-style selected visibility toggle hides selected entities");

    // --- hierarchy pickability state is editor-only and recursive -----------
    context.SetSceneUnpickable(scene, entity.GetUUID(), true, true);
    HK_CHECK_MSG(!context.IsScenePickable(entity.GetUUID()), "pickability toggle locks parent");
    HK_CHECK_MSG(!context.IsScenePickable(child.GetUUID()), "pickability toggle includes descendants");
    context.SetSceneUnpickable(scene, entity.GetUUID(), false, false);
    HK_CHECK_MSG(context.IsScenePickable(entity.GetUUID()), "Alt-style pickability toggle affects only parent");
    HK_CHECK_MSG(!context.IsScenePickable(child.GetUUID()), "child pickability state is preserved by single toggle");

    context.SetSceneUnpickable(scene, entity.GetUUID(), false, true);
    context.SelectSceneEntity(entity.GetUUID());
    context.ToggleSceneEntitySelection(child.GetUUID());
    context.ToggleSelectedPickability(scene);
    HK_CHECK_MSG(!context.IsScenePickable(entity.GetUUID()) && !context.IsScenePickable(child.GetUUID()),
                 "L-style selected pickability toggle locks selected entities");
    HK_CHECK_MSG(context.selection.Empty(), "locking selected entities removes them from selection");

    context.SelectSceneEntity(entity.GetUUID());
    HK_CHECK_MSG(context.selection.Empty(), "locked entities cannot be selected through editor context");
    context.SetSceneUnpickable(scene, entity.GetUUID(), false, true);
    context.SelectSceneEntity(entity.GetUUID());
    HK_CHECK_MSG(context.selection.IsSelected(entity.GetUUID()), "unlocked entities can be selected through editor context");
    context.SetSceneUnpickable(scene, entity.GetUUID(), true, false);
    HK_CHECK_MSG(!context.selection.IsSelected(entity.GetUUID()), "locking one selected entity removes only that entity");
    HK_CHECK_MSG(context.selection.Empty(), "selection is empty after locking its last entity");
    context.SetSceneUnpickable(scene, entity.GetUUID(), false, true);
    context.SetSceneUnpickable(scene, child.GetUUID(), true, false);
    context.SelectSceneEntity(entity.GetUUID());
    context.SelectSceneRange({entity.GetUUID(), child.GetUUID(), sibling.GetUUID()}, sibling.GetUUID(), false);
    HK_CHECK_MSG(context.selection.IsSelected(entity.GetUUID()) && context.selection.IsSelected(sibling.GetUUID()),
                 "range selection still selects pickable rows around a locked row");
    HK_CHECK_MSG(!context.selection.IsSelected(child.GetUUID()), "range selection skips locked rows");

    // --- editor-only state is not serialized into scene files ---------------
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "hk_editor_only_state.scene.yaml";
    std::error_code ec;
    std::filesystem::remove(path, ec);
    SceneSerializer serializer(scene);
    HK_CHECK_MSG(static_cast<bool>(serializer.Serialize(path)), "scene with editor state serializes");
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    const std::string yaml = out.str();
    HK_CHECK_MSG(yaml.find("SceneHidden") == std::string::npos, "hidden ids are not serialized");
    HK_CHECK_MSG(yaml.find("SceneUnpickable") == std::string::npos, "unpickable ids are not serialized");
    std::filesystem::remove(path, ec);
}

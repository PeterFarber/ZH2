#include "Test.hpp"

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
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
}

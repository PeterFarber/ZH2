#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class EditorCommand;
class Scene;

// Plain transform payload for transform commands (local-space TRS).
struct TransformData {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};

// Editor-side serialization helpers built on the ECS ComponentSerializer. These
// capture/restore entities (and subtrees) as YAML so commands and the clipboard
// can undo destructive edits or instantiate copies. "Restore" preserves the
// original UUIDs (used by undo of delete/duplicate); "Instantiate" remaps to
// fresh UUIDs (used by paste).
namespace EntitySnapshot {

// Serializes one entity (id + every component) as a YAML map. Empty when the id
// is not in the scene.
std::string CaptureEntity(Scene& scene, UUID id);

// Serializes the subtree rooted at 'rootId' as a YAML sequence (root first,
// then descendants depth-first). Empty when the root is missing.
std::string CaptureSubtree(Scene& scene, UUID rootId);

// Re-applies a single-entity YAML map onto the existing entity, restoring its
// component data without changing the entity set's identity.
void ApplyEntity(Scene& scene, const std::string& yaml);

// Recreates a captured subtree, preserving the original UUIDs and component
// data, relinking the root under 'parentId' (invalid => scene root). Returns the
// restored root UUID (invalid on failure).
UUID RestoreSubtree(Scene& scene, const std::string& yaml, UUID parentId);

// Instantiates a captured subtree with freshly generated UUIDs, parented under
// 'parentId' (invalid => scene root). Returns the new root UUID.
UUID InstantiateSubtree(Scene& scene, const std::string& yaml, UUID parentId);

} // namespace EntitySnapshot

// Factory functions returning ready-to-execute commands. Concrete command types
// live in the .cpp so the header stays light. Pass the result to
// UndoRedoStack::Execute.
namespace EditorCommands {

std::unique_ptr<EditorCommand> CreateEntity(std::string name, UUID parentId = UUID(0));
std::unique_ptr<EditorCommand> DeleteEntity(Scene& scene, UUID entityId);
std::unique_ptr<EditorCommand> DuplicateEntity(UUID sourceId);

// Builds entities in the scene and returns the root UUIDs it created (one or
// more). Runs once on first execution; the spawned subtrees are then snapshotted
// so the whole spawn is a single undo/redo step.
using SpawnBuilder = std::function<std::vector<UUID>(Scene&)>;

// Wraps a builder as a single undoable "spawn" command. The created roots are
// selected on execute. Used by the placement tools (lights, hockey markers).
std::unique_ptr<EditorCommand> SpawnEntities(std::string actionName, SpawnBuilder builder);

// Instantiates a .prefab.yaml into the active scene with fresh UUIDs, optionally
// parented under 'parentId' (invalid => scene root). The instantiated root is
// selected; the whole instancing is one undo/redo step.
std::unique_ptr<EditorCommand> InstantiatePrefab(std::filesystem::path prefabPath, UUID parentId = UUID(0));

std::unique_ptr<EditorCommand> RenameEntity(UUID entityId, std::string oldName, std::string newName);
std::unique_ptr<EditorCommand> SetActive(UUID entityId, bool oldValue, bool newValue);
std::unique_ptr<EditorCommand> SetParent(Scene& scene, UUID childId, UUID newParentId);
std::unique_ptr<EditorCommand> TransformEntity(UUID entityId, const TransformData& oldValue,
                                               const TransformData& newValue);

std::unique_ptr<EditorCommand> AddComponent(UUID entityId, std::string componentName);
std::unique_ptr<EditorCommand> RemoveComponent(Scene& scene, UUID entityId, std::string componentName);
std::unique_ptr<EditorCommand> EditComponentField(UUID entityId, std::string label, std::string beforeYaml,
                                                  std::string afterYaml);

// Clipboard-backed commands. PasteEntity instantiates a captured subtree with
// fresh UUIDs under 'parentId' (invalid => root). PasteComponent applies a
// captured component (one-key YAML map) onto an existing entity.
std::unique_ptr<EditorCommand> PasteEntity(std::string subtreeYaml, UUID parentId);
std::unique_ptr<EditorCommand> PasteComponent(Scene& scene, UUID targetId, std::string componentName,
                                              std::string componentYaml);

} // namespace EditorCommands

} // namespace Hockey

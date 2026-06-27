#pragma once

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

struct FieldMetadata;
class AssetManager;
class Scene;

namespace FieldDrawers {

// Outcome of drawing a single field for one frame. 'changed' tracks live value
// edits (apply immediately for responsive UI); 'started'/'committed' bracket an
// interaction so the inspector can capture a before-snapshot and push one undo
// command per drag/edit rather than per frame.
struct FieldEdit {
    bool changed = false;
    bool started = false;
    bool committed = false;
};

// Returns a pointer to the raw field bytes inside a component instance
// (componentData + field.offset). Exposed for unit testing the offset-based
// access independently of ImGui.
void* FieldPointer(void* componentData, const FieldMetadata& field);

// Assigns an entity-reference UUID field through metadata. Returns true only
// when the field accepts entity references and the value actually changed.
bool AssignEntityReference(const FieldMetadata& field, void* componentData, UUID entityId);

// Draws an editable ImGui widget for a single field based on its metadata
// (type, range/speed, enum names, read-only). Read-only fields never report a
// change. `assetManager` (optional) is used by FieldType::AssetRef fields to
// resolve asset ids to names and accept asset drag-drop payloads; pass null in
// headless contexts (AssetRef then shows the raw id, read-only).
FieldEdit Draw(const FieldMetadata& field,
               void* componentData,
               AssetManager* assetManager = nullptr,
               Scene* scene = nullptr);

} // namespace FieldDrawers

} // namespace Hockey

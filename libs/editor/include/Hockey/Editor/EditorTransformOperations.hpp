#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Editor/EditorCommands.hpp"

namespace Hockey {

class Scene;
class Selection;

namespace EditorTransformOperations {

std::vector<UUID> TopLevelTransformableSelection(Scene& scene, const Selection& selection);
std::vector<EntityTransformSnapshot> CaptureLocalSnapshots(Scene& scene, const std::vector<UUID>& entityIds);
void CaptureCurrentAsAfter(Scene& scene, std::vector<EntityTransformSnapshot>& snapshots);
std::vector<EntityTransformSnapshot> ChangedSnapshots(const std::vector<EntityTransformSnapshot>& snapshots);
void ApplyWorldTranslationDelta(Scene& scene, const std::vector<UUID>& entityIds, const glm::vec3& delta);

} // namespace EditorTransformOperations

} // namespace Hockey

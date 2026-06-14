#pragma once

#include <vector>

#include "Hockey/ECS/SceneValidator.hpp"

namespace Hockey {

class Scene;

// ---------------------------------------------------------------------------
// Physics-specific scene checks. These run as an external SceneValidator hook
// (registered by RegisterPhysicsComponents) so the editor's Scene Validation
// panel and the headless server both surface physics setup problems without
// the ECS depending on hockey_physics.
// ---------------------------------------------------------------------------
void ValidatePhysicsScene(const Scene& scene, std::vector<SceneValidationIssue>& issues);

// Registers ValidatePhysicsScene with SceneValidator. Idempotent.
void RegisterPhysicsValidation();

} // namespace Hockey

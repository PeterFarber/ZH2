#pragma once

#include <vector>

#include "Hockey/ECS/SceneValidator.hpp"

namespace Hockey {

class Scene;

void ValidateGameplayScene(const Scene& scene, std::vector<SceneValidationIssue>& issues);
void RegisterGameplayValidation();

}

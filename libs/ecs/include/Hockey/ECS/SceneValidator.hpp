#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Hockey/Core/UUID.hpp"

namespace Hockey {

class Scene;

struct SceneValidationIssue {
    enum class Severity { Warning, Error };

    Severity severity = Severity::Warning;
    std::string message;
    UUID entityId{};
};

// Stateless structural/semantic checks over a scene. Errors mean the scene is
// broken; warnings flag suspicious-but-loadable hockey-scene content.
//
// Libraries layered above the ECS (e.g. hockey_physics) can contribute their
// own checks via RegisterExternal() without the ECS depending on them; external
// validators run after the built-in checks and append to the same issue list.
class SceneValidator {
public:
    using ValidatorFn = std::function<void(const Scene&, std::vector<SceneValidationIssue>&)>;

    static std::vector<SceneValidationIssue> Validate(const Scene& scene);
    static bool HasErrors(const std::vector<SceneValidationIssue>& issues);

    static void RegisterExternal(ValidatorFn validator);
    static void ClearExternal();
};

} // namespace Hockey

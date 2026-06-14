#include "Hockey/ECS/SceneValidator.hpp"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

namespace {

using Severity = SceneValidationIssue::Severity;

void AddIssue(std::vector<SceneValidationIssue>& issues, Severity severity, std::string message, UUID entityId = {}) {
    issues.push_back({severity, std::move(message), entityId});
}

std::vector<SceneValidator::ValidatorFn>& ExternalValidators() {
    static std::vector<SceneValidator::ValidatorFn> validators;
    return validators;
}

} // namespace

std::vector<SceneValidationIssue> SceneValidator::Validate(const Scene& scene) {
    std::vector<SceneValidationIssue> issues;
    const entt::registry& registry = scene.Registry();

    // Pass 1: collect identities and detect duplicate UUIDs.
    std::unordered_map<std::uint64_t, entt::entity> handleById;
    auto idView = registry.view<const IDComponent>();
    for (const entt::entity handle : idView) {
        const UUID id = idView.get<const IDComponent>(handle).id;
        if (id.Value() == 0) {
            AddIssue(issues, Severity::Error, "Entity has a zero/invalid UUID", id);
            continue;
        }
        if (handleById.find(id.Value()) != handleById.end()) {
            AddIssue(issues, Severity::Error, "Duplicate entity UUID " + id.ToString(), id);
            continue;
        }
        handleById.emplace(id.Value(), handle);
    }

    const auto exists = [&](UUID id) { return handleById.find(id.Value()) != handleById.end(); };

    int goalCount = 0;
    bool hasPuck = false;
    bool hasSkaterSpawn = false;
    bool hasGoalieSpawn = false;

    // Pass 2: per-entity structural and semantic checks.
    for (const entt::entity handle : idView) {
        const UUID id = idView.get<const IDComponent>(handle).id;

        if (!registry.all_of<NameComponent>(handle)) {
            AddIssue(issues, Severity::Error, "Entity missing NameComponent", id);
        }
        if (!registry.all_of<TransformComponent>(handle)) {
            AddIssue(issues, Severity::Error, "Entity missing TransformComponent", id);
        }
        if (!registry.all_of<ChildrenComponent>(handle)) {
            AddIssue(issues, Severity::Error, "Entity missing ChildrenComponent", id);
        }

        if (const auto* parent = registry.try_get<ParentComponent>(handle)) {
            if (!exists(parent->parentId)) {
                AddIssue(issues, Severity::Error, "ParentComponent references a missing entity", id);
            } else {
                const entt::entity parentHandle = handleById.at(parent->parentId.Value());
                const auto* parentChildren = registry.try_get<ChildrenComponent>(parentHandle);
                bool listed = false;
                if (parentChildren != nullptr) {
                    for (const UUID childId : parentChildren->children) {
                        if (childId == id) {
                            listed = true;
                            break;
                        }
                    }
                }
                if (!listed) {
                    AddIssue(issues, Severity::Error, "Parent/child link mismatch: parent does not list child", id);
                }
            }
        }

        if (const auto* children = registry.try_get<ChildrenComponent>(handle)) {
            for (const UUID childId : children->children) {
                if (!exists(childId)) {
                    AddIssue(issues, Severity::Error, "ChildrenComponent references a missing entity", id);
                    continue;
                }
                const entt::entity childHandle = handleById.at(childId.Value());
                const auto* childParent = registry.try_get<ParentComponent>(childHandle);
                if (childParent == nullptr || childParent->parentId != id) {
                    AddIssue(issues, Severity::Error, "Parent/child link mismatch: child does not point back", id);
                }
            }
        }

        if (const auto* spawn = registry.try_get<SpawnPointComponent>(handle)) {
            if (spawn->index < 0) {
                AddIssue(issues, Severity::Error, "Spawn point index is negative", id);
            }
            if (spawn->team == Team::None) {
                AddIssue(issues, Severity::Warning, "Spawn point has no team assigned", id);
            }
            if (spawn->role == PlayerRole::Skater) {
                hasSkaterSpawn = true;
            } else if (spawn->role == PlayerRole::Goalie) {
                hasGoalieSpawn = true;
            }
        }

        if (const auto* faceoff = registry.try_get<FaceoffSpotComponent>(handle)) {
            if (faceoff->index < 0) {
                AddIssue(issues, Severity::Error, "Faceoff spot index is negative", id);
            }
        }

        if (const auto* goal = registry.try_get<GoalComponent>(handle)) {
            ++goalCount;
            if (goal->defendingTeam == Team::None) {
                AddIssue(issues, Severity::Error, "Goal defending team must be Home or Away", id);
            }
        }

        if (registry.all_of<PuckComponent>(handle)) {
            hasPuck = true;
        }
    }

    // Cycle detection over parent links.
    {
        std::unordered_set<std::uint64_t> reported;
        for (const auto& [idValue, handle] : handleById) {
            std::unordered_set<std::uint64_t> path;
            entt::entity current = handle;
            while (true) {
                const std::uint64_t currentId = registry.get<const IDComponent>(current).id.Value();
                if (!path.insert(currentId).second) {
                    if (reported.insert(currentId).second) {
                        AddIssue(issues, Severity::Error, "Cyclic hierarchy detected", UUID(currentId));
                    }
                    break;
                }
                const auto* parent = registry.try_get<ParentComponent>(current);
                if (parent == nullptr) {
                    break;
                }
                const auto it = handleById.find(parent->parentId.Value());
                if (it == handleById.end()) {
                    break;
                }
                current = it->second;
            }
        }
    }

    // Hockey-scene content warnings (non-fatal).
    if (!hasPuck) {
        AddIssue(issues, Severity::Warning, "Scene has no puck marker (PuckComponent)");
    }
    if (goalCount < 2) {
        AddIssue(issues, Severity::Warning, "Scene has fewer than 2 goals");
    } else if (goalCount > 2) {
        AddIssue(issues, Severity::Warning, "Scene has more than 2 goals");
    }
    if (!hasSkaterSpawn) {
        AddIssue(issues, Severity::Warning, "Scene has no skater spawn markers");
    }
    if (!hasGoalieSpawn) {
        AddIssue(issues, Severity::Warning, "Scene has no goalie spawn markers");
    }

    // External validators (e.g. physics) append their own checks.
    for (const auto& validator : ExternalValidators()) {
        if (validator) {
            validator(scene, issues);
        }
    }

    return issues;
}

void SceneValidator::RegisterExternal(ValidatorFn validator) {
    ExternalValidators().push_back(std::move(validator));
}

void SceneValidator::ClearExternal() {
    ExternalValidators().clear();
}

bool SceneValidator::HasErrors(const std::vector<SceneValidationIssue>& issues) {
    for (const auto& issue : issues) {
        if (issue.severity == Severity::Error) {
            return true;
        }
    }
    return false;
}

} // namespace Hockey

#include "Test.hpp"

#include <string>

#include <entt/entt.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneValidator.hpp"

using namespace Hockey;

namespace {

bool HasWarningContaining(const std::vector<SceneValidationIssue>& issues, const std::string& substr) {
    for (const auto& issue : issues) {
        if (issue.severity == SceneValidationIssue::Severity::Warning &&
            issue.message.find(substr) != std::string::npos) {
            return true;
        }
    }
    return false;
}

Entity MakeSpawn(Scene& scene, const std::string& name, Team team, PlayerRole role) {
    Entity e = scene.CreateEntity(name);
    e.AddComponent<SpawnPointComponent>(SpawnPointComponent{team, role, 0});
    return e;
}

} // namespace

void RunSceneValidationTests() {
    HockeyTest::BeginSuite("SceneValidationTests");

    // Valid hockey scene -> no errors.
    {
        Scene scene("Valid");
        Entity homeGoal = scene.CreateEntity("Home Goal");
        homeGoal.AddComponent<GoalComponent>(GoalComponent{Team::Home});
        Entity awayGoal = scene.CreateEntity("Away Goal");
        awayGoal.AddComponent<GoalComponent>(GoalComponent{Team::Away});
        Entity puck = scene.CreateEntity("Puck");
        puck.AddComponent<PuckComponent>(PuckComponent{true});
        MakeSpawn(scene, "Skater", Team::Home, PlayerRole::Skater);
        MakeSpawn(scene, "Goalie", Team::Home, PlayerRole::Goalie);

        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(!SceneValidator::HasErrors(issues));
    }

    // Duplicate UUID -> error.
    {
        Scene scene("Dup");
        Entity a = scene.CreateEntity("A");
        Entity b = scene.CreateEntity("B");
        scene.Registry().get<IDComponent>(b.Handle()).id = a.GetUUID();
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }

    // Broken parent reference -> error.
    {
        Scene scene("BrokenParent");
        Entity a = scene.CreateEntity("A");
        scene.Registry().emplace<ParentComponent>(a.Handle(), ParentComponent{UUID(987654321ULL)});
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }

    // Broken child reference -> error.
    {
        Scene scene("BrokenChild");
        Entity a = scene.CreateEntity("A");
        a.GetComponent<ChildrenComponent>().children.push_back(UUID(135791113ULL));
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }

    // Cyclic hierarchy -> error.
    {
        Scene scene("Cycle");
        Entity a = scene.CreateEntity("A");
        Entity b = scene.CreateEntity("B");
        entt::registry& reg = scene.Registry();
        reg.emplace<ParentComponent>(a.Handle(), ParentComponent{b.GetUUID()});
        reg.emplace<ParentComponent>(b.Handle(), ParentComponent{a.GetUUID()});
        a.GetComponent<ChildrenComponent>().children.push_back(b.GetUUID());
        b.GetComponent<ChildrenComponent>().children.push_back(a.GetUUID());
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }

    // Missing puck and missing goals -> warnings.
    {
        Scene scene("Empty");
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(HasWarningContaining(issues, "puck"));
        HK_CHECK(HasWarningContaining(issues, "goals"));
    }

    // Negative spawn index -> error.
    {
        Scene scene("BadSpawn");
        Entity e = scene.CreateEntity("Spawn");
        e.AddComponent<SpawnPointComponent>(SpawnPointComponent{Team::Home, PlayerRole::Skater, -1});
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }
}

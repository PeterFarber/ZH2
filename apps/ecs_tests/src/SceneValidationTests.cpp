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

Entity MakeSpawn(Scene& scene, const std::string& name, Team team, bool faceoffSpawn = false) {
    Entity e = scene.CreateEntity(name);
    SpawnPointComponent spawn;
    spawn.team = team;
    spawn.faceoffSpawn = faceoffSpawn;
    e.AddComponent<SpawnPointComponent>(spawn);
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
        MakeSpawn(scene, "Skater", Team::Home);
        MakeSpawn(scene, "Goalie", Team::Home);

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

    // Neutral normal spawns warn because they cannot place a team.
    {
        Scene scene("SpawnNoTeam");
        MakeSpawn(scene, "Spawn", Team::None, false);
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(HasWarningContaining(issues, "normal spawn point has no team"));
    }

    // More than two goals -> warning.
    {
        Scene scene("TooManyGoals");
        for (int i = 0; i < 3; ++i) {
            Entity g = scene.CreateEntity("Goal" + std::to_string(i));
            g.AddComponent<GoalComponent>(GoalComponent{Team::Home});
        }
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(HasWarningContaining(issues, "more than 2 goals"));
    }

    // Goal with no defending team -> error.
    {
        Scene scene("GoalNoTeam");
        Entity g = scene.CreateEntity("Goal");
        g.AddComponent<GoalComponent>(GoalComponent{Team::None});
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(SceneValidator::HasErrors(issues));
    }

    // Neutral faceoff spawns are valid authoring data.
    {
        Scene scene("NeutralFaceoffSpawn");
        MakeSpawn(scene, "Neutral Faceoff Spawn", Team::None, true);
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(!SceneValidator::HasErrors(issues));
    }

    // Missing normal team spawn markers -> warnings.
    {
        Scene scene("NoSpawns");
        const auto issues = SceneValidator::Validate(scene);
        HK_CHECK(HasWarningContaining(issues, "skater spawn"));
        HK_CHECK(HasWarningContaining(issues, "goalie spawn"));
    }
}

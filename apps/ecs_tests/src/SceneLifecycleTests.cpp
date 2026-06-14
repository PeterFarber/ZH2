#include "Test.hpp"

#include <memory>

#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/System.hpp"

using namespace Hockey;

namespace {

class CountingSystem : public System {
public:
    int starts = 0;
    int stops = 0;
    int updates = 0;
    int fixedUpdates = 0;

    void OnStart(Scene&) override {
        ++starts;
    }
    void OnStop(Scene&) override {
        ++stops;
    }
    void OnUpdate(Scene&, float) override {
        ++updates;
    }
    void OnFixedUpdate(Scene&, float) override {
        ++fixedUpdates;
    }
};

} // namespace

void RunSceneLifecycleTests() {
    HockeyTest::BeginSuite("SceneLifecycleTests");

    Scene scene("Test");

    auto system = std::make_unique<CountingSystem>();
    CountingSystem* raw = system.get();
    scene.AddSystem(std::move(system));
    HK_CHECK_EQ(scene.SystemCount(), static_cast<std::size_t>(1));

    scene.OnRuntimeStart();
    HK_CHECK_EQ(raw->starts, 1);

    scene.OnUpdate(0.016f);
    HK_CHECK_EQ(raw->updates, 1);

    scene.OnFixedUpdate(0.016f);
    HK_CHECK_EQ(raw->fixedUpdates, 1);

    scene.OnRuntimeStop();
    HK_CHECK_EQ(raw->stops, 1);

    scene.OnSimulationStart();
    HK_CHECK_EQ(raw->starts, 2);

    scene.OnSimulationStop();
    HK_CHECK_EQ(raw->stops, 2);

    scene.ClearSystems();
    HK_CHECK_EQ(scene.SystemCount(), static_cast<std::size_t>(0));
}

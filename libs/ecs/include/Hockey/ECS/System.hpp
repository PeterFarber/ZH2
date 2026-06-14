#pragma once

namespace Hockey {

class Scene;

// Base interface for scene-attached systems. Phase 2 ships no concrete systems;
// the interface exists so later phases (gameplay, physics, ...) can hook into
// the scene lifecycle without the ECS layer knowing what they do.
class System {
public:
    virtual ~System() = default;

    virtual void OnStart(Scene& scene) {
        (void)scene;
    }
    virtual void OnStop(Scene& scene) {
        (void)scene;
    }
    virtual void OnUpdate(Scene& scene, float deltaTime) {
        (void)scene;
        (void)deltaTime;
    }
    virtual void OnFixedUpdate(Scene& scene, float fixedDeltaTime) {
        (void)scene;
        (void)fixedDeltaTime;
    }
};

} // namespace Hockey

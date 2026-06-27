#pragma once

#include "Hockey/Core/Result.hpp"
#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

class Scene;

class StickAttachment {
public:
    static Status EnsureStickAttached(Scene& scene, Entity player);
};

} // namespace Hockey

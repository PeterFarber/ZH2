#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

class Scene;

// High-level prefab operations. Serialization details live in PrefabSerializer.
class Prefab {
public:
    // Saves the subtree rooted at 'root' to a ".prefab.yaml" file.
    Status SaveFromEntity(Scene& scene, Entity root, const std::filesystem::path& path);

    // Instantiates a prefab file into 'targetScene' with fresh UUIDs.
    Result<Entity> Instantiate(Scene& targetScene, const std::filesystem::path& path);
};

} // namespace Hockey

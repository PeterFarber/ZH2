#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

class Scene;

// Reads/writes prefab files and performs UUID-remapped instancing.
class PrefabSerializer {
public:
    static constexpr int kPrefabVersion = 1;

    static Status Save(Scene& scene, Entity root, const std::filesystem::path& path);

    static Result<Entity> Instantiate(Scene& targetScene, const std::filesystem::path& path);
};

} // namespace Hockey

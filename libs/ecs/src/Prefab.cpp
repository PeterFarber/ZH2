#include "Hockey/ECS/Prefab.hpp"

#include "Hockey/ECS/PrefabSerializer.hpp"

namespace Hockey {

Status Prefab::SaveFromEntity(Scene& scene, Entity root, const std::filesystem::path& path) {
    return PrefabSerializer::Save(scene, root, path);
}

Result<Entity> Prefab::Instantiate(Scene& targetScene, const std::filesystem::path& path) {
    return PrefabSerializer::Instantiate(targetScene, path);
}

} // namespace Hockey

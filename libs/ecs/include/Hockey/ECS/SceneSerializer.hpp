#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"

namespace Hockey {

class Scene;

// Reads/writes a whole scene to a human-readable ".scene.yaml" file.
class SceneSerializer {
public:
    static constexpr int kSceneVersion = 1;

    explicit SceneSerializer(Scene& scene);

    Status Serialize(const std::filesystem::path& path);
    Status Deserialize(const std::filesystem::path& path);

private:
    Scene& m_Scene;
};

} // namespace Hockey

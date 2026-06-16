#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/ECS/Entity.hpp"

namespace Hockey {

class Scene;
class PrefabOverrideSet;

// Reads/writes prefab files and performs UUID-remapped instancing.
class PrefabSerializer {
public:
    static constexpr int kPrefabVersion = 1;

    static Status Save(Scene& scene, Entity root, const std::filesystem::path& path);
    // Same as Save, but reports the generated prefab asset id so callers can link
    // the source entity to the new prefab (Unity-style "create prefab").
    static Status Save(Scene& scene, Entity root, const std::filesystem::path& path, UUID& outAssetId);

    static Result<Entity> Instantiate(Scene& targetScene, const std::filesystem::path& path);

    // --- prefab override workflow (Unity-style apply/revert) ----------------
    // These operate on a prefab *instance* (an entity stamped with a
    // PrefabComponent). `instanceRoot` must be the instance's root entity.

    // Records which fields of the instance subtree diverge from the linked
    // prefab's authored values. Clears `outSet` first, then appends one
    // PrefabOverride per diverging field. Returns the number of overrides found
    // (0 means the instance matches the prefab). Only fields PrefabOverrideSet
    // can re-apply are considered, so the result always round-trips.
    static Result<int> ComputeOverrides(Scene& scene, Entity instanceRoot, PrefabOverrideSet& outSet);

    // Writes the instance subtree back to its linked prefab file, preserving the
    // prefab's asset id ("Apply Overrides"). The instance's per-entity source ids
    // are refreshed so subsequent reverts stay matched.
    static Status ApplyInstanceToPrefab(Scene& scene, Entity instanceRoot);

    // Restores the instance subtree's component values from its linked prefab
    // file ("Revert Overrides"), keeping each instance entity's id, hierarchy and
    // prefab link. Components added on the instance but absent in the prefab are
    // left untouched.
    static Status RevertInstanceToPrefab(Scene& scene, Entity instanceRoot);
};

} // namespace Hockey

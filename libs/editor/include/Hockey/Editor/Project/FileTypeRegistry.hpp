#pragma once

#include <filesystem>

namespace Hockey {

// Coarse classification of project files for the asset browser. The editor only
// fully understands a handful of types; the rest are listed but not actionable
// (no import/cooking in Phase 4).
enum class EditorFileType {
    Unknown,
    Folder,
    Scene,        // *.scene.yaml
    Prefab,       // *.prefab.yaml
    Material,     // *.material.yaml
    UIScreen,     // *.rml
    UITheme,      // *.rcss
    ClientFlow,   // *.clientflow.yaml
    Toml,         // *.toml
    ShaderSource, // *.glsl/.vert/.frag/.comp
    ShaderBinary, // *.spv
    Image,        // *.png/.jpg/... (imported as textures)
    Model,        // *.gltf/.glb (imported as models/meshes)
    Text,         // *.yaml/.json/.txt/.md
};

struct FileTypeInfo {
    EditorFileType type = EditorFileType::Unknown;
    const char* label = "File"; // short human-readable type label
    bool supported = false;     // editor can act on this type (open/instantiate)
};

// Stateless extension-based classifier. Handles the compound ".scene.yaml" and
// ".prefab.yaml" suffixes before falling back to the final extension.
class FileTypeRegistry {
public:
    static FileTypeInfo Classify(const std::filesystem::path& path);

    static bool IsScene(const std::filesystem::path& path);
    static bool IsPrefab(const std::filesystem::path& path);
};

} // namespace Hockey

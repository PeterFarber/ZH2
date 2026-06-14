#pragma once
#include <filesystem>
#include <string>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/AssetType.hpp"

namespace Hockey {

// Cross-platform path helpers for the asset pipeline. All stored paths are
// project-relative and use forward slashes (generic format) so databases and
// sidecars are portable between Windows and Linux.
namespace AssetPath {

// Lexically normalized, forward-slash representation.
std::filesystem::path Normalize(const std::filesystem::path& path);

// Convert an absolute (or relative) path into a project-relative generic-format
// path under `root`. If `path` is not under `root`, the normalized path is
// returned unchanged.
std::filesystem::path ToProjectRelative(const std::filesystem::path& root,
                                        const std::filesystem::path& path);

// Resolve a (typically project-relative) path against `root` into an absolute
// path. Already-absolute inputs are returned normalized.
std::filesystem::path ToAbsolute(const std::filesystem::path& root,
                                 const std::filesystem::path& path);

// True when `path` is located inside `root` (after normalization).
bool IsUnderRoot(const std::filesystem::path& root, const std::filesystem::path& path);

// Metadata sidecar path for a raw asset: "<raw>.meta.yaml".
std::filesystem::path MetadataSidecar(const std::filesystem::path& rawPath);

// True if a path is itself a metadata sidecar.
bool IsMetadataSidecar(const std::filesystem::path& path);

// Sub-directory name used under data/cooked/assets/ for a given type.
std::string CookedSubdirectory(AssetType type);

// File extension (including dot) used for cooked output of a given type.
std::string CookedExtension(AssetType type);

// Project-relative cooked output path for an asset, e.g.
// "data/cooked/assets/textures/<id>.tex.bin". `cookedRoot` should be
// project-relative (e.g. "data/cooked").
std::filesystem::path CookedPath(const std::filesystem::path& cookedRoot, AssetType type,
                                 AssetID id);

} // namespace AssetPath
} // namespace Hockey

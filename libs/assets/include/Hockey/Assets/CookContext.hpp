#pragma once
#include <filesystem>

#include "Hockey/Assets/AssetMetadata.hpp"

namespace Hockey {

class AssetDatabase;

// Inputs handed to a Cooker. The cooker reads the raw file referenced by
// `metadata.rawPath` and writes a cooked artifact under `cookedRoot`.
struct CookContext {
    AssetMetadata metadata;
    std::filesystem::path rawRoot;    // absolute data/raw root
    std::filesystem::path cookedRoot; // absolute data/cooked root
    std::filesystem::path projectRoot;
    AssetDatabase* database = nullptr;
};

} // namespace Hockey

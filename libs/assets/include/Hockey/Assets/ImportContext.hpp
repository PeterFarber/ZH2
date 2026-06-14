#pragma once
#include <filesystem>

#include "Hockey/Assets/AssetID.hpp"

namespace Hockey {

class AssetDatabase;

// Inputs handed to an Importer. Paths are absolute so importers can read files
// directly; the importer records project-relative paths into the metadata.
struct ImportContext {
    std::filesystem::path rawPath;    // absolute path to the raw source file
    std::filesystem::path rawRoot;    // absolute data/raw root
    std::filesystem::path cookedRoot; // absolute data/cooked root
    std::filesystem::path projectRoot;
    AssetID existingId;               // reuse this id if valid (preserves identity)
    AssetDatabase* database = nullptr;
};

} // namespace Hockey

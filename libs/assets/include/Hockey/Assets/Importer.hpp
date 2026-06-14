#pragma once
#include <string>

#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/ImportContext.hpp"
#include "Hockey/Assets/ImportResult.hpp"

namespace Hockey {

// An importer turns a raw source file into asset metadata (and possibly several
// generated sub-assets). Importers do not produce cooked output; that is the
// cooker's job. Importers are registered with the AssetManager by extension.
class Importer {
public:
    virtual ~Importer() = default;

    virtual bool SupportsExtension(const std::string& extension) const = 0;
    virtual AssetType GetAssetType() const = 0;
    virtual std::string Version() const = 0;
    virtual ImportResult Import(const ImportContext& context) = 0;
};

} // namespace Hockey

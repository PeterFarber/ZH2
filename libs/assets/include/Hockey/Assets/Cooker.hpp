#pragma once
#include <string>

#include "Hockey/Assets/AssetType.hpp"
#include "Hockey/Assets/CookContext.hpp"
#include "Hockey/Assets/CookResult.hpp"

namespace Hockey {

// A cooker turns an imported asset into an engine-ready cooked artifact written
// under data/cooked. Cookers are registered with the AssetManager by AssetType.
class Cooker {
public:
    virtual ~Cooker() = default;

    virtual AssetType GetAssetType() const = 0;
    virtual std::string Version() const = 0;
    virtual CookResult Cook(const CookContext& context) = 0;
};

} // namespace Hockey

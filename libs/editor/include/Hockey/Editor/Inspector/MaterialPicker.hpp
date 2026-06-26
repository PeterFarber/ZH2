#pragma once

#include <cstdint>

namespace Hockey {

class AssetManager;

// A material chooser used by the inspector. Materials are authored assets; a
// zero id means no material asset is assigned.
namespace MaterialPicker {

struct Result {
    bool changed = false;   // value changed this frame (mark scene dirty)
    bool committed = false; // discrete selection finished (push one undo step)
};

// Draws a combo listing authored material assets known to `assetManager`, plus
// a "(none)" entry. `label` is shown after the combo. `assetManager` may be null.
Result Draw(const char* label, std::uint64_t& materialAsset, AssetManager* assetManager);

} // namespace MaterialPicker

} // namespace Hockey

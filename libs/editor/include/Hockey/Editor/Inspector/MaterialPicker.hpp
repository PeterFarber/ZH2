#pragma once

#include <cstdint>
#include <string>

namespace Hockey {

class AssetManager;

// A unified material chooser used by the inspector. MeshRendererComponent stores
// a material two ways: an authored asset id (materialAsset) and a built-in name
// fallback (materialName, e.g. "BuiltIn.Ice"). The renderer prefers the asset id
// when set, so editing the two fields independently is error-prone. This control
// folds them into a single drop-down that always keeps exactly one of them
// populated.
namespace MaterialPicker {

struct Result {
    bool changed = false;   // value changed this frame (mark scene dirty)
    bool committed = false; // discrete selection finished (push one undo step)
};

// Draws the picker: a combo listing the built-in materials and every authored
// material asset known to `assetManager`, plus a "(default)" entry. Selecting a
// built-in sets `materialName` and clears `materialAsset`; selecting an asset (or
// dropping one from the project tree) sets `materialAsset` and clears
// `materialName`. `label` is shown after the combo. `assetManager` may be null
// (then only built-ins are offered).
Result Draw(const char* label, std::uint64_t& materialAsset, std::string& materialName, AssetManager* assetManager);

} // namespace MaterialPicker

} // namespace Hockey

#include "Hockey/Assets/CookedFormat.hpp"

namespace Hockey {
namespace CookedFormat {

CookedAssetHeader MakeHeader(AssetType type, AssetID id, uint64_t sourceHash, uint32_t version) {
    CookedAssetHeader header;
    header.magic = kMagic;
    header.version = version;
    header.assetType = static_cast<uint32_t>(type);
    header.assetID = id.Value();
    header.sourceHash = sourceHash;
    return header;
}

} // namespace CookedFormat
} // namespace Hockey

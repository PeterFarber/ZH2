#include "Test.hpp"

#include "Hockey/Assets/AssetHandle.hpp"
#include "Hockey/Assets/AssetID.hpp"

#include <unordered_map>

using namespace Hockey;

void RunAssetIDTests() {
    HockeyTest::BeginSuite("AssetIDTests");

    const AssetID a = AssetID::Generate();
    const AssetID b = AssetID::Generate();

    HK_CHECK_MSG(a.IsValid(), "generated id is valid");
    HK_CHECK_MSG(a.Value() != 0, "generated id is nonzero");
    HK_CHECK_MSG(a != b, "two generated ids differ");
    HK_CHECK_MSG(!a.ToString().empty(), "id string non-empty");

    const AssetID invalid;
    HK_CHECK_MSG(!invalid.IsValid(), "default id invalid");
    HK_CHECK_EQ(invalid.Value(), 0ull);

    std::unordered_map<AssetID, int> map;
    map[a] = 1;
    map[b] = 2;
    HK_CHECK_EQ(map[a], 1);
    HK_CHECK_EQ(map[b], 2);
    HK_CHECK_EQ(static_cast<int>(map.size()), 2);

    const AssetID copy(a.Value());
    HK_CHECK_MSG(copy == a, "reconstructed id compares equal");

    // Typed handles wrap an id and report validity.
    const TextureAssetHandle handle(a);
    HK_CHECK_MSG(handle.IsValid(), "handle valid");
    HK_CHECK_MSG(handle.ID() == a, "handle keeps id");
    const MeshAssetHandle emptyHandle;
    HK_CHECK_MSG(!emptyHandle.IsValid(), "default handle invalid");
}

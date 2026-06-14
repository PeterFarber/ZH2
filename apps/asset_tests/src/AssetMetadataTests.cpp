#include "Test.hpp"

#include "Hockey/Assets/AssetMetadata.hpp"
#include "Hockey/Assets/Serialization/AssetMetadataSerializer.hpp"

using namespace Hockey;

void RunAssetMetadataTests() {
    HockeyTest::BeginSuite("AssetMetadataTests");

    AssetMetadata source;
    source.id = AssetID(123456789ull);
    source.type = AssetType::Texture;
    source.name = "ice_base_color";
    source.importerVersion = "TextureImporter_v1";
    source.cookerVersion = "TextureCooker_v1";
    source.rawPath = "data/raw/textures/ice_base_color.png";
    source.cookedPath = "data/cooked/assets/textures/123456789.tex.bin";
    source.rawFileTimestamp = 111;
    source.rawFileHash = 222;
    source.importedTimestamp = 333;
    source.cookedTimestamp = 444;
    source.dependencies = {AssetID(10), AssetID(20)};
    source.dependents = {AssetID(30)};
    source.dirty = false;
    source.missing = true;
    source.cooked = true;

    const std::string text = AssetMetadataSerializer::Serialize(source);
    HK_CHECK_MSG(!text.empty(), "serialized metadata non-empty");

    AssetMetadata loaded;
    HK_CHECK_MSG(AssetMetadataSerializer::Deserialize(text, loaded), "metadata deserializes");

    HK_CHECK_MSG(loaded.id == source.id, "id preserved");
    HK_CHECK_MSG(loaded.type == AssetType::Texture, "type preserved");
    HK_CHECK_EQ(loaded.name, std::string("ice_base_color"));
    HK_CHECK_EQ(loaded.importerVersion, std::string("TextureImporter_v1"));
    HK_CHECK_EQ(loaded.rawPath.generic_string(), source.rawPath.generic_string());
    HK_CHECK_EQ(loaded.cookedPath.generic_string(), source.cookedPath.generic_string());
    HK_CHECK_EQ(loaded.rawFileHash, 222ull);
    HK_CHECK_EQ(loaded.cookedTimestamp, 444ull);

    HK_CHECK_EQ(static_cast<int>(loaded.dependencies.size()), 2);
    HK_CHECK_MSG(loaded.dependencies[0] == AssetID(10), "dependency 0 preserved");
    HK_CHECK_MSG(loaded.dependencies[1] == AssetID(20), "dependency 1 preserved");
    HK_CHECK_EQ(static_cast<int>(loaded.dependents.size()), 1);
    HK_CHECK_MSG(loaded.dependents[0] == AssetID(30), "dependent preserved");

    HK_CHECK_MSG(loaded.dirty == false, "dirty flag preserved");
    HK_CHECK_MSG(loaded.missing == true, "missing flag preserved");
    HK_CHECK_MSG(loaded.cooked == true, "cooked flag preserved");
}

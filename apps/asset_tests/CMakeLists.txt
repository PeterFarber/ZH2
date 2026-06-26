add_executable(HockeyAssetTests
    src/AssetTestsMain.cpp
    src/AssetIDTests.cpp
    src/AssetMetadataTests.cpp
    src/AssetPathTests.cpp
    src/AssetDatabaseTests.cpp
    src/TextureImportTests.cpp
    src/GltfImportTests.cpp
    src/MaterialTests.cpp
    src/BasicShapeAssetTests.cpp
    src/ShaderCookTests.cpp
    src/DependencyTests.cpp
    src/HotReloadTests.cpp
    src/Phase5GapTests.cpp
)

target_link_libraries(HockeyAssetTests PRIVATE hockey_core hockey_assets)
set_target_properties(HockeyAssetTests PROPERTIES OUTPUT_NAME "HockeyAssetTests")

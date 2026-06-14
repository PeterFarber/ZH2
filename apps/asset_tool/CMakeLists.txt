add_executable(HockeyAssetTool
    src/AssetToolMain.cpp
    src/AssetToolCommands.cpp
)

target_link_libraries(HockeyAssetTool PRIVATE hockey_core hockey_assets)
set_target_properties(HockeyAssetTool PROPERTIES OUTPUT_NAME "HockeyAssetTool")

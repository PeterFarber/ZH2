add_executable(HockeyRendererTests
    src/RendererTestsMain.cpp
    src/RendererSettingsTests.cpp
    src/ResourceHandleTests.cpp
    src/ShaderCompileTests.cpp
    src/RenderGraphTests.cpp
    src/RendererShadowContractTests.cpp
    src/RendererDecalContractTests.cpp
    src/MaterialAssetContractTests.cpp
    src/RendererSmokeTests.cpp
    src/UIOverlayContractTests.cpp
    src/HeadlessServerLinkTests.cpp
)

target_link_libraries(HockeyRendererTests PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets)
set_target_properties(HockeyRendererTests PROPERTIES OUTPUT_NAME "HockeyRendererTests")

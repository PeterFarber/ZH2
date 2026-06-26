add_executable(HockeyUITests
    src/UITestsMain.cpp
    src/UISettingsTests.cpp
    src/ClientFlowTests.cpp
        src/UIAssetContractTests.cpp
        src/UIArchitectureContractTests.cpp
        src/RmlUiInterfaceTests.cpp
        src/RmlUiContextTests.cpp
        src/UIInputMapperTests.cpp
)

target_link_libraries(HockeyUITests PRIVATE hockey_core hockey_renderer hockey_ui)
set_target_properties(HockeyUITests PROPERTIES OUTPUT_NAME "HockeyUITests")

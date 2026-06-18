add_executable(HockeyShaderTool
    src/ShaderToolMain.cpp
)

target_link_libraries(HockeyShaderTool PRIVATE hockey_core hockey_renderer)
set_target_properties(HockeyShaderTool PROPERTIES OUTPUT_NAME "HockeyShaderTool")

include("${CMAKE_SOURCE_DIR}/cmake/HockeyEmbedTextResource.cmake")

add_executable(HockeyDedicatedServer
    src/DedicatedServerMain.cpp
    src/DedicatedServerApp.cpp
    src/DedicatedServerApp.hpp
    src/EmbeddedServerDefaults.hpp
)

target_include_directories(HockeyDedicatedServer PRIVATE src)
target_link_libraries(HockeyDedicatedServer PRIVATE hockey_core hockey_ecs hockey_physics hockey_gameplay)
set_target_properties(HockeyDedicatedServer PROPERTIES OUTPUT_NAME "HockeyDedicatedServer")

hockey_embed_text_resource(
    HockeyDedicatedServer
    "${CMAKE_SOURCE_DIR}/data/config/editor.toml"
    "EmbeddedServerDefaults.hpp"
    "HockeyServer::Embedded"
    "DefaultServerConfigToml"
    "EmbeddedServerDefaults"
)

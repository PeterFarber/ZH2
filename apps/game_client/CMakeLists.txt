include("${CMAKE_SOURCE_DIR}/cmake/HockeyEmbedTextResource.cmake")

add_executable(HockeyGameClient
    src/GameClientMain.cpp
    src/GameClientApp.cpp
    src/GameClientApp.hpp
    src/EmbeddedClientDefaults.hpp
    src/GameplayCamera.cpp
    src/GameplayCamera.hpp
    src/GameplayPresentation.cpp
    src/GameplayPresentation.hpp
)

target_include_directories(HockeyGameClient PRIVATE src)
target_link_libraries(HockeyGameClient PRIVATE hockey_core hockey_ecs hockey_renderer hockey_assets hockey_physics hockey_gameplay hockey_ui)
set_target_properties(HockeyGameClient PROPERTIES OUTPUT_NAME "HockeyGameClient")

hockey_embed_text_resource(
    HockeyGameClient
    "${CMAKE_SOURCE_DIR}/data/config/editor.toml"
    "EmbeddedClientDefaults.hpp"
    "HockeyClient::Embedded"
    "DefaultClientConfigToml"
    "EmbeddedClientDefaults"
)

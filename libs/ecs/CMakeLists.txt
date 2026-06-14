find_package(EnTT CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(hockey_ecs STATIC)

target_sources(hockey_ecs
    PRIVATE
        src/Entity.cpp
        src/Scene.cpp
        src/Transform.cpp
        src/Hierarchy.cpp
        src/ComponentMetadata.cpp
        src/ComponentRegistry.cpp
        src/ComponentSerializer.cpp
        src/RenderComponents.cpp
        src/SceneSerializer.cpp
        src/SceneValidator.cpp
        src/SceneEvents.cpp
        src/Prefab.cpp
        src/PrefabSerializer.cpp
        src/PrefabOverride.cpp
        src/System.cpp
        src/YAML.cpp
)

target_include_directories(hockey_ecs
    PUBLIC include
)

target_link_libraries(hockey_ecs
    PUBLIC
        hockey_core
        EnTT::EnTT
        yaml-cpp::yaml-cpp
)

if(MSVC)
    target_compile_options(hockey_ecs PRIVATE /W4)
else()
    target_compile_options(hockey_ecs PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_ecs PRIVATE /WX)
    else()
        target_compile_options(hockey_ecs PRIVATE -Werror)
    endif()
endif()

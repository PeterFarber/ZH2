# hockey_assets: the content pipeline (asset identity, metadata, database,
# import/cook framework, runtime CPU-side loading, hot-reload watcher).
#
# Architecture rule: hockey_assets depends ONLY on hockey_core. It must never
# link the renderer, editor, ECS, physics, networking, or gameplay. The
# renderer/editor depend on hockey_assets, not the other way around.

find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(lunasvg CONFIG REQUIRED)

add_library(hockey_assets STATIC)

target_sources(hockey_assets
    PRIVATE
        src/AssetID.cpp
        src/AssetType.cpp
        src/AssetPath.cpp
        src/AssetHash.cpp
        src/AssetDatabase.cpp
        src/AssetEvent.cpp
        src/AssetWatcher.cpp
        src/RuntimePackage.cpp
        src/AssetManager.cpp
        src/CookedFormat.cpp
        src/StbImpl.cpp
        src/Gltf/GltfLoader.cpp
        src/Assets/AnimationAsset.cpp
        src/Assets/SkeletonAsset.cpp
        src/Assets/TextureAsset.cpp
        src/Assets/ShaderAsset.cpp
        src/Importers/TextureImporter.cpp
        src/Importers/MaterialImporter.cpp
        src/Importers/GltfImporter.cpp
        src/Importers/ShaderImporter.cpp
        src/Importers/SceneImporter.cpp
        src/Cookers/AnimationCooker.cpp
        src/Cookers/TextureCooker.cpp
        src/Cookers/MaterialCooker.cpp
        src/Cookers/MeshCooker.cpp
        src/Cookers/ModelCooker.cpp
        src/Cookers/SkeletonCooker.cpp
        src/Cookers/ShaderCooker.cpp
        src/Cookers/SceneCooker.cpp
        src/Runtime/AssetLoader.cpp
        src/Runtime/AnimationLoader.cpp
        src/Runtime/SkeletonLoader.cpp
        src/Runtime/TextureLoader.cpp
        src/Runtime/SvgRasterizer.cpp
        src/Runtime/MeshLoader.cpp
        src/Runtime/ModelLoader.cpp
        src/Runtime/MaterialLoader.cpp
        src/Runtime/ShaderLoader.cpp
        src/Serialization/MetadataYaml.cpp
        src/Serialization/MaterialSerializer.cpp
        src/Serialization/SceneDependencies.cpp
        src/Serialization/AssetMetadataSerializer.cpp
        src/Serialization/AssetDatabaseSerializer.cpp
)

target_include_directories(hockey_assets
    PUBLIC include
    PRIVATE src ${Stb_INCLUDE_DIR}
)

target_link_libraries(hockey_assets
    PUBLIC
        hockey_core
        glm::glm
        yaml-cpp::yaml-cpp
    PRIVATE
        fastgltf::fastgltf
        meshoptimizer::meshoptimizer
        unofficial::shaderc::shaderc
        lunasvg::lunasvg
)

if(MSVC)
    target_compile_options(hockey_assets PRIVATE /W4)
else()
    target_compile_options(hockey_assets PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_assets PRIVATE /WX)
    else()
        target_compile_options(hockey_assets PRIVATE -Werror)
    endif()
endif()

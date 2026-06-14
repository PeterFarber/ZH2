# hockey_renderer: shared Vulkan renderer used by the game client and map
# editor. Depends on hockey_core, hockey_ecs and hockey_assets (to upload cooked
# meshes/materials/textures to the GPU). The dedicated server must never link
# this target.

find_package(volk CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(hockey_renderer STATIC)

target_sources(hockey_renderer
    PRIVATE
        # Public / backend-agnostic
        src/RenderTypes.cpp
        src/RendererSettings.cpp
        src/Renderer.cpp
        src/Camera.cpp
        src/Light.cpp
        src/DebugDraw.cpp
        src/Shader.cpp
        src/Pipeline.cpp
        src/DescriptorSet.cpp
        src/Mesh.cpp
        src/Material.cpp
        src/RenderTarget.cpp
        src/RenderPass.cpp
        src/RenderGraph.cpp
        # Vulkan base
        src/Vulkan/VolkImpl.cpp
        src/Vulkan/VmaImpl.cpp
        src/Vulkan/VulkanCommon.cpp
        src/Vulkan/VulkanInstance.cpp
        src/Vulkan/VulkanDebug.cpp
        src/Vulkan/VulkanSurface.cpp
        src/Vulkan/VulkanDevice.cpp
        src/Vulkan/VulkanAllocator.cpp
        src/Vulkan/VulkanSwapchain.cpp
        src/Vulkan/VulkanCommand.cpp
        src/Vulkan/VulkanFrameData.cpp
        src/Vulkan/VulkanBuffer.cpp
        src/Vulkan/VulkanTexture.cpp
        src/Vulkan/VulkanSampler.cpp
        src/Vulkan/VulkanShader.cpp
        src/Vulkan/VulkanPipeline.cpp
        src/Vulkan/VulkanDescriptor.cpp
        src/Vulkan/VulkanMesh.cpp
        src/Vulkan/VulkanRenderGraph.cpp
        src/Vulkan/VulkanFrameTargets.cpp
        src/Vulkan/VulkanPostProcess.cpp
)

target_include_directories(hockey_renderer
    PUBLIC include
    PRIVATE ${Stb_INCLUDE_DIR}
)

# All Vulkan dependencies are PRIVATE: public headers expose only opaque
# handles (pimpl), so no Vulkan type leaks to consumers.
target_link_libraries(hockey_renderer
    PUBLIC
        hockey_core
        hockey_ecs
    PRIVATE
        hockey_assets
        volk::volk_headers
        Vulkan::Headers
        GPUOpen::VulkanMemoryAllocator
        unofficial::shaderc::shaderc
)

# Force volk to use the loader at runtime (no direct link to the Vulkan
# loader); function pointers are resolved via volkInitialize.
target_compile_definitions(hockey_renderer PRIVATE VK_NO_PROTOTYPES)

if(MSVC)
    target_compile_options(hockey_renderer PRIVATE /W4)
else()
    target_compile_options(hockey_renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_renderer PRIVATE /WX)
    else()
        target_compile_options(hockey_renderer PRIVATE -Werror)
    endif()
endif()

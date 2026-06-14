#pragma once

#include <filesystem>

#include "Hockey/Renderer/RendererSettings.hpp"

namespace Hockey {

class Window;

struct RendererInitInfo {
    // Window providing the Vulkan surface. May be null for headless creation
    // (e.g. render-target-only usage); a swapchain is only created when set.
    Window* window = nullptr;

    RendererSettings settings;

    bool enableValidation = false;
    bool enableDebugMarkers = true;

    // Where GLSL sources live and where compiled SPIR-V is cached/written.
    std::filesystem::path shaderSourceDirectory;
    std::filesystem::path shaderBinaryDirectory;
};

} // namespace Hockey

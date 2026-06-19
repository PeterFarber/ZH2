#pragma once

#include <cstdint>
#include <vector>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Shader.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

// A compiled shader stage ready to be referenced by a pipeline.
struct VulkanShaderModule {
    VkShaderModule module = VK_NULL_HANDLE;
    ShaderStage stage = ShaderStage::Vertex;

    bool IsValid() const {
        return module != VK_NULL_HANDLE;
    }
};

VkShaderModule CreateShaderModule(const RenderDevice& device, const std::vector<uint32_t>& spirv);
void DestroyShaderModule(const RenderDevice& device, VkShaderModule module);

// Loads a cached SPIR-V binary when up to date, otherwise compiles GLSL and
// creates a shader module from the result.
Result<VulkanShaderModule> LoadShaderModule(const RenderDevice& device, const ShaderDesc& desc,
                                            const std::filesystem::path& binaryDir = {});

} // namespace Hockey

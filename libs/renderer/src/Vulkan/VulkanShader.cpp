#include "Hockey/Renderer/Vulkan/VulkanShader.hpp"

namespace Hockey {

VkShaderModule CreateShaderModule(const RenderDevice& device, const std::vector<uint32_t>& spirv) {
    if (spirv.empty()) {
        return VK_NULL_HANDLE;
    }
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = spirv.size() * sizeof(uint32_t);
    info.pCode = spirv.data();

    VkShaderModule module = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device.device, &info, nullptr, &module) != VK_SUCCESS) {
        HK_CORE_ERROR("vkCreateShaderModule failed");
        return VK_NULL_HANDLE;
    }
    return module;
}

void DestroyShaderModule(const RenderDevice& device, VkShaderModule module) {
    if (module != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device.device, module, nullptr);
    }
}

Result<VulkanShaderModule> LoadShaderModule(const RenderDevice& device, const ShaderDesc& desc,
                                            const std::filesystem::path& binaryDir) {
    Result<CompiledShaderData> compiled = CompileShaderFile(desc, binaryDir);
    if (!compiled) {
        return Result<VulkanShaderModule>::Fail(compiled.error);
    }
    VulkanShaderModule out;
    out.stage = desc.stage;
    out.module = CreateShaderModule(device, compiled.value.spirv);
    if (out.module == VK_NULL_HANDLE) {
        return Result<VulkanShaderModule>::Fail("Failed to create shader module: " + desc.sourcePath.string());
    }
    return Result<VulkanShaderModule>::Ok(out);
}

} // namespace Hockey

#include "Hockey/Renderer/Vulkan/VulkanShader.hpp"

#include <fstream>
#include <utility>

namespace Hockey {

namespace {

std::filesystem::path ShaderBinaryPath(const ShaderDesc& desc, const std::filesystem::path& binaryDir) {
    if (binaryDir.empty()) {
        return {};
    }
    return binaryDir / (desc.sourcePath.filename().generic_string() + ".spv");
}

bool IsShaderBinaryUpToDate(const ShaderDesc& desc, const std::filesystem::path& binaryPath) {
    std::error_code ec;
    if (binaryPath.empty() || !std::filesystem::exists(binaryPath, ec) || ec) {
        return false;
    }

    const auto binaryTime = std::filesystem::last_write_time(binaryPath, ec);
    if (ec) {
        return false;
    }

    const auto sourceTime = std::filesystem::last_write_time(desc.sourcePath, ec);
    if (ec || sourceTime > binaryTime) {
        return false;
    }

    // The current renderer shader graph uses common.glsl as the shared include.
    // Keep this conservative so editing common.glsl invalidates every shader.
    const std::filesystem::path commonPath = desc.sourcePath.parent_path() / "common.glsl";
    if (std::filesystem::exists(commonPath, ec) && !ec) {
        const auto commonTime = std::filesystem::last_write_time(commonPath, ec);
        if (ec || commonTime > binaryTime) {
            return false;
        }
    }

    return true;
}

Result<CompiledShaderData> LoadCachedShader(const std::filesystem::path& binaryPath) {
    std::ifstream file(binaryPath, std::ios::binary | std::ios::ate);
    if (!file) {
        return Result<CompiledShaderData>::Fail("Failed to open cached shader: " + binaryPath.string());
    }

    const std::streamsize byteSize = static_cast<std::streamsize>(file.tellg());
    if (byteSize <= 0 || byteSize % static_cast<std::streamsize>(sizeof(uint32_t)) != 0) {
        return Result<CompiledShaderData>::Fail("Invalid cached shader size: " + binaryPath.string());
    }

    CompiledShaderData data;
    data.spirv.resize(static_cast<size_t>(byteSize) / sizeof(uint32_t));
    data.outputPath = binaryPath;

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.spirv.data()), byteSize);
    if (!file) {
        return Result<CompiledShaderData>::Fail("Failed to read cached shader: " + binaryPath.string());
    }

    return Result<CompiledShaderData>::Ok(std::move(data));
}

} // namespace

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
    const std::filesystem::path binaryPath = ShaderBinaryPath(desc, binaryDir);
    Result<CompiledShaderData> compiled = IsShaderBinaryUpToDate(desc, binaryPath) ? LoadCachedShader(binaryPath) :
                                                                                    CompileShaderFile(desc, binaryDir);
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

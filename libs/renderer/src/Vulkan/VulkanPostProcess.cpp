#include "Hockey/Renderer/Vulkan/VulkanPostProcess.hpp"

#include "Hockey/Renderer/DescriptorSet.hpp"
#include "Hockey/Renderer/Shader.hpp"
#include "Hockey/Renderer/Vulkan/VulkanShader.hpp"

namespace Hockey {

namespace {

constexpr uint32_t kPostPushSize = 128;

VulkanPipeline MakeFullscreenPipeline(const RenderDevice& device, VkPipelineLayout layout, VkShaderModule vertex,
                                      VkShaderModule fragment, VkFormat colorFormat, BlendMode blend,
                                      const char* name) {
    GraphicsPipelineDesc desc;
    desc.vertexShader = vertex;
    desc.fragmentShader = fragment;
    desc.layout = layout;
    desc.colorFormats = {colorFormat};
    desc.depthFormat = VK_FORMAT_UNDEFINED;
    desc.topology = PrimitiveTopology::TriangleList;
    desc.cullMode = CullMode::None;
    desc.depthTest = false;
    desc.depthWrite = false;
    desc.blend = blend;
    desc.debugName = name;
    VulkanPipeline pipeline = CreateGraphicsPipeline(device, desc);
    pipeline.layout = layout;
    pipeline.ownsLayout = false;
    return pipeline;
}

Result<VkShaderModule> LoadFragment(const RenderDevice& device, const std::filesystem::path& src,
                                    const std::filesystem::path& bin, const char* file) {
    ShaderDesc desc;
    desc.sourcePath = src / file;
    desc.stage = ShaderStage::Fragment;
    Result<VulkanShaderModule> module = LoadShaderModule(device, desc, bin);
    if (!module) {
        return Result<VkShaderModule>::Fail(module.error);
    }
    return Result<VkShaderModule>::Ok(module.value.module);
}

} // namespace

Status VulkanPostProcess::Create(const RenderDevice& device) {
    m_Device = &device;

    m_PerPassLayout = CreateDescriptorSetLayout(device, PerPassSetLayoutDesc());
    if (m_PerPassLayout == VK_NULL_HANDLE) {
        return Status::Fail("Failed to create per-pass descriptor layout");
    }

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    push.offset = 0;
    push.size = kPostPushSize;
    m_Layout = CreatePipelineLayout(device, {m_PerPassLayout}, {push});
    if (m_Layout == VK_NULL_HANDLE) {
        return Status::Fail("Failed to create post-process pipeline layout");
    }

    for (VulkanDescriptorAllocator& alloc : m_Alloc) {
        if (Status s = alloc.Create(device, 64); !s) {
            return s;
        }
    }
    return Status::Ok();
}

Status VulkanPostProcess::BuildPipelines(const std::filesystem::path& shaderSrc, const std::filesystem::path& shaderBin,
                                         VkFormat hdrFormat, VkFormat ldrFormat, VkFormat aoFormat,
                                         VkFormat swapchainFormat) {
    const RenderDevice& device = *m_Device;

    ShaderDesc vsDesc;
    vsDesc.sourcePath = shaderSrc / "fullscreen_triangle.vert";
    vsDesc.stage = ShaderStage::Vertex;
    Result<VulkanShaderModule> vs = LoadShaderModule(device, vsDesc, shaderBin);
    if (!vs) {
        return Status::Fail(vs.error);
    }
    const VkShaderModule vertex = vs.value.module;

    struct Build {
        const char* file;
        VkFormat format;
        BlendMode blend;
        const char* name;
        VulkanPipeline* out;
    };
    const Build builds[] = {
        {"tonemap.frag", ldrFormat, BlendMode::Opaque, "TonemapPipeline", &m_Tonemap},
        {"bloom_downsample.frag", hdrFormat, BlendMode::Opaque, "BloomDownPipeline", &m_BloomDown},
        {"bloom_upsample.frag", hdrFormat, BlendMode::Additive, "BloomUpPipeline", &m_BloomUp},
        {"ssao.frag", aoFormat, BlendMode::Opaque, "SSAOPipeline", &m_Ssao},
        {"ssao_blur.frag", aoFormat, BlendMode::Opaque, "SSAOBlurPipeline", &m_SsaoBlur},
        {"fxaa.frag", swapchainFormat, BlendMode::Opaque, "FXAAPipeline", &m_Fxaa},
    };

    Status status = Status::Ok();
    for (const Build& build : builds) {
        Result<VkShaderModule> fragment = LoadFragment(device, shaderSrc, shaderBin, build.file);
        if (!fragment) {
            status = Status::Fail(fragment.error);
            break;
        }
        *build.out =
            MakeFullscreenPipeline(device, m_Layout, vertex, fragment.value, build.format, build.blend, build.name);
        DestroyShaderModule(device, fragment.value);
        if (!build.out->IsValid()) {
            status = Status::Fail(std::string("Failed to build post pipeline: ") + build.name);
            break;
        }
    }

    DestroyShaderModule(device, vertex);
    return status;
}

void VulkanPostProcess::BeginFrame(uint32_t frameIndex) {
    m_Alloc[frameIndex].Reset();
}

void VulkanPostProcess::Draw(VkCommandBuffer cmd, uint32_t frameIndex, VkPipeline pipeline, VkImageView input,
                             VkSampler sampler, const void* push, uint32_t pushSize) {
    VkDescriptorSet set = m_Alloc[frameIndex].Allocate(m_PerPassLayout);
    DescriptorWriter writer;
    writer.WriteImage(0, input, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                      DescriptorType::CombinedImageSampler);
    writer.Update(*m_Device, set);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, 0, 1, &set, 0, nullptr);
    if (push != nullptr && pushSize > 0) {
        vkCmdPushConstants(cmd, m_Layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushSize, push);
    }
    vkCmdDraw(cmd, 3, 1, 0, 0);
}

void VulkanPostProcess::Destroy() {
    if (m_Device == nullptr) {
        return;
    }
    DestroyPipeline(*m_Device, m_Tonemap);
    DestroyPipeline(*m_Device, m_BloomDown);
    DestroyPipeline(*m_Device, m_BloomUp);
    DestroyPipeline(*m_Device, m_Ssao);
    DestroyPipeline(*m_Device, m_SsaoBlur);
    DestroyPipeline(*m_Device, m_Fxaa);
    for (VulkanDescriptorAllocator& alloc : m_Alloc) {
        alloc.Destroy();
    }
    if (m_Layout != VK_NULL_HANDLE) {
        DestroyPipelineLayout(*m_Device, m_Layout);
        m_Layout = VK_NULL_HANDLE;
    }
    if (m_PerPassLayout != VK_NULL_HANDLE) {
        DestroyDescriptorSetLayout(*m_Device, m_PerPassLayout);
        m_PerPassLayout = VK_NULL_HANDLE;
    }
    m_Device = nullptr;
}

} // namespace Hockey

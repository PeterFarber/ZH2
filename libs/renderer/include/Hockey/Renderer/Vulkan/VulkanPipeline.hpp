#pragma once

#include <vector>

#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommon.hpp"

namespace Hockey {

struct VulkanPipeline {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE; // owned only if ownsLayout is true
    bool ownsLayout = false;

    bool IsValid() const {
        return pipeline != VK_NULL_HANDLE;
    }
};

// Describes the vertex buffer layout fed into a graphics pipeline.
struct VertexInputDesc {
    uint32_t stride = 0;
    std::vector<VkVertexInputAttributeDescription> attributes;
};

struct GraphicsPipelineDesc {
    VkShaderModule vertexShader = VK_NULL_HANDLE;
    VkShaderModule fragmentShader = VK_NULL_HANDLE; // optional (depth/shadow may omit)
    const char* vertexEntry = "main";
    const char* fragmentEntry = "main";

    VkPipelineLayout layout = VK_NULL_HANDLE;
    VertexInputDesc vertexInput;

    // Dynamic-rendering attachment formats.
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    CullMode cullMode = CullMode::Back;
    bool depthTest = true;
    bool depthWrite = true;
    VkCompareOp depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
    bool depthBias = false; // shadow passes
    float depthBiasConstantFactor = 1.25f;
    float depthBiasSlopeFactor = 1.75f;
    BlendMode blend = BlendMode::Opaque;

    const char* debugName = nullptr;
};

VkPipelineLayout CreatePipelineLayout(const RenderDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts,
                                      const std::vector<VkPushConstantRange>& pushConstants);
void DestroyPipelineLayout(const RenderDevice& device, VkPipelineLayout layout);

VulkanPipeline CreateGraphicsPipeline(const RenderDevice& device, const GraphicsPipelineDesc& desc);
void DestroyPipeline(const RenderDevice& device, VulkanPipeline& pipeline);

// Common vertex input layouts.
VertexInputDesc MeshVertexInput();         // position/normal/tangent/uv
VertexInputDesc PositionOnlyVertexInput(); // depth + shadow passes
VertexInputDesc DebugLineVertexInput();    // position + color

} // namespace Hockey

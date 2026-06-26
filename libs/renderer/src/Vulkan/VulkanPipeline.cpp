#include "Hockey/Renderer/Vulkan/VulkanPipeline.hpp"

#include <array>
#include <cstddef>

#include "Hockey/Renderer/DebugDraw.hpp"
#include "Hockey/Renderer/Mesh.hpp"

namespace Hockey {

VkPipelineLayout CreatePipelineLayout(const RenderDevice& device, const std::vector<VkDescriptorSetLayout>& setLayouts,
                                      const std::vector<VkPushConstantRange>& pushConstants) {
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
    info.pSetLayouts = setLayouts.empty() ? nullptr : setLayouts.data();
    info.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
    info.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device.device, &info, nullptr, &layout) != VK_SUCCESS) {
        HK_CORE_ERROR("vkCreatePipelineLayout failed");
        return VK_NULL_HANDLE;
    }
    return layout;
}

void DestroyPipelineLayout(const RenderDevice& device, VkPipelineLayout layout) {
    if (layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.device, layout, nullptr);
    }
}

VulkanPipeline CreateGraphicsPipeline(const RenderDevice& device, const GraphicsPipelineDesc& desc) {
    VulkanPipeline result;
    result.layout = desc.layout;

    std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
    uint32_t stageCount = 0;
    stages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[stageCount].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[stageCount].module = desc.vertexShader;
    stages[stageCount].pName = desc.vertexEntry;
    ++stageCount;
    if (desc.fragmentShader != VK_NULL_HANDLE) {
        stages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[stageCount].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[stageCount].module = desc.fragmentShader;
        stages[stageCount].pName = desc.fragmentEntry;
        ++stageCount;
    }

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = desc.vertexInput.stride;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (desc.vertexInput.stride > 0 && !desc.vertexInput.attributes.empty()) {
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &binding;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(desc.vertexInput.attributes.size());
        vertexInput.pVertexAttributeDescriptions = desc.vertexInput.attributes.data();
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = ToVkTopology(desc.topology);

    VkPipelineViewportStateCreateInfo viewport{};
    viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = ToVkCullMode(desc.cullMode);
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;
    if (desc.depthBias) {
        raster.depthBiasEnable = VK_TRUE;
        raster.depthBiasConstantFactor = desc.depthBiasConstantFactor;
        raster.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;
    }

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = desc.depthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = desc.depthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = desc.depthCompare;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    switch (desc.blend) {
    case BlendMode::Opaque:
        blendAttachment.blendEnable = VK_FALSE;
        break;
    case BlendMode::AlphaBlend:
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case BlendMode::PremultipliedAlpha:
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    case BlendMode::Additive:
        blendAttachment.blendEnable = VK_TRUE;
        blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        break;
    }

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(std::max<size_t>(desc.colorFormats.size(), 0),
                                                                      blendAttachment);

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
    colorBlend.pAttachments = blendAttachments.empty() ? nullptr : blendAttachments.data();

    const std::array<VkDynamicState, 2> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamic.pDynamicStates = dynamicStates.data();

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = static_cast<uint32_t>(desc.colorFormats.size());
    renderingInfo.pColorAttachmentFormats = desc.colorFormats.empty() ? nullptr : desc.colorFormats.data();
    renderingInfo.depthAttachmentFormat = desc.depthFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = stageCount;
    pipelineInfo.pStages = stages.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewport;
    pipelineInfo.pRasterizationState = &raster;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamic;
    pipelineInfo.layout = desc.layout;

    if (vkCreateGraphicsPipelines(device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &result.pipeline) !=
        VK_SUCCESS) {
        HK_CORE_ERROR("vkCreateGraphicsPipelines failed ({})", desc.debugName ? desc.debugName : "unnamed");
        result.pipeline = VK_NULL_HANDLE;
        return result;
    }
    if (desc.debugName != nullptr) {
        device.SetObjectName(reinterpret_cast<uint64_t>(result.pipeline), VK_OBJECT_TYPE_PIPELINE, desc.debugName);
    }
    return result;
}

void DestroyPipeline(const RenderDevice& device, VulkanPipeline& pipeline) {
    if (pipeline.pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device.device, pipeline.pipeline, nullptr);
    }
    if (pipeline.ownsLayout && pipeline.layout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device.device, pipeline.layout, nullptr);
    }
    pipeline = VulkanPipeline{};
}

VertexInputDesc MeshVertexInput() {
    VertexInputDesc desc;
    desc.stride = sizeof(Vertex);
    desc.attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, normal))},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, tangent))},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, uv))},
    };
    return desc;
}

VertexInputDesc PositionOnlyVertexInput() {
    VertexInputDesc desc;
    desc.stride = sizeof(Vertex);
    desc.attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, position))},
    };
    return desc;
}

VertexInputDesc DebugLineVertexInput() {
    VertexInputDesc desc;
    desc.stride = sizeof(DebugDraw::LineVertex);
    desc.attributes = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(DebugDraw::LineVertex, position))},
        {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<uint32_t>(offsetof(DebugDraw::LineVertex, color))},
    };
    return desc;
}

} // namespace Hockey

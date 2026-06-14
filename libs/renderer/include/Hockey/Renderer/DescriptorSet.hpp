#pragma once

#include <cstdint>
#include <vector>

namespace Hockey {

// Backend-agnostic descriptor declarations. The Vulkan layer (VulkanDescriptor)
// turns these into VkDescriptorSetLayouts / pools / sets.

enum class DescriptorType {
    UniformBuffer,
    UniformBufferDynamic,
    CombinedImageSampler,
    StorageBuffer,
    StorageImage,
};

// Shader stage bitmask for descriptor visibility.
enum ShaderStageMask : uint32_t {
    Stage_Vertex = 1u << 0,
    Stage_Fragment = 1u << 1,
    Stage_Compute = 1u << 2,
    Stage_AllGraphics = Stage_Vertex | Stage_Fragment,
};

// Update frequency / set index convention used across the renderer:
//   set 0 = Global (camera, scene/lights, shadow map)
//   set 1 = Material (material UBO + PBR textures)
//   set 2 = PerPass (post-process inputs)
enum class DescriptorFrequency : uint32_t {
    Global = 0,
    Material = 1,
    PerPass = 2,
};

struct DescriptorBinding {
    uint32_t binding = 0;
    DescriptorType type = DescriptorType::UniformBuffer;
    uint32_t stages = Stage_AllGraphics;
    uint32_t count = 1;
};

struct DescriptorSetLayoutDesc {
    DescriptorFrequency frequency = DescriptorFrequency::Global;
    std::vector<DescriptorBinding> bindings;
};

// The standard layouts the renderer's shaders expect.
DescriptorSetLayoutDesc GlobalSetLayoutDesc();
DescriptorSetLayoutDesc MaterialSetLayoutDesc();
DescriptorSetLayoutDesc PerPassSetLayoutDesc();

} // namespace Hockey

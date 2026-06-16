#include "Hockey/Renderer/DescriptorSet.hpp"

namespace Hockey {

DescriptorSetLayoutDesc GlobalSetLayoutDesc() {
    DescriptorSetLayoutDesc desc;
    desc.frequency = DescriptorFrequency::Global;
    desc.bindings = {
        {0, DescriptorType::UniformBuffer, Stage_AllGraphics, 1},     // camera
        {1, DescriptorType::UniformBuffer, Stage_Fragment, 1},        // scene / lights
        {2, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // directional shadow map
        {3, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // screen-space AO
        {4, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // local (spot/point) shadow map
    };
    return desc;
}

DescriptorSetLayoutDesc MaterialSetLayoutDesc() {
    DescriptorSetLayoutDesc desc;
    desc.frequency = DescriptorFrequency::Material;
    desc.bindings = {
        {0, DescriptorType::UniformBuffer, Stage_Fragment, 1},        // material params
        {1, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // base color
        {2, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // normal
        {3, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // metallic-roughness
        {4, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // occlusion
        {5, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // emissive
    };
    return desc;
}

DescriptorSetLayoutDesc PerPassSetLayoutDesc() {
    DescriptorSetLayoutDesc desc;
    desc.frequency = DescriptorFrequency::PerPass;
    desc.bindings = {
        {0, DescriptorType::CombinedImageSampler, Stage_Fragment, 1}, // input image
    };
    return desc;
}

} // namespace Hockey

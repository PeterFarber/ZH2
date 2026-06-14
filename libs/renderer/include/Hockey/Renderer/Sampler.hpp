#pragma once

#include "Hockey/Renderer/RenderTypes.hpp"

namespace Hockey {

struct SamplerDesc {
    FilterMode minFilter = FilterMode::Linear;
    FilterMode magFilter = FilterMode::Linear;
    AddressMode addressU = AddressMode::Repeat;
    AddressMode addressV = AddressMode::Repeat;
    AddressMode addressW = AddressMode::Repeat;
    float maxAnisotropy = 16.0f;
    bool compareEnable = false; // for shadow samplers
};

} // namespace Hockey

#include "Hockey/Renderer/RenderTarget.hpp"

namespace Hockey {

const char* LoadOpToString(LoadOp op) {
    switch (op) {
    case LoadOp::Load:
        return "Load";
    case LoadOp::Clear:
        return "Clear";
    case LoadOp::DontCare:
        return "DontCare";
    }
    return "Unknown";
}

const char* StoreOpToString(StoreOp op) {
    switch (op) {
    case StoreOp::Store:
        return "Store";
    case StoreOp::DontCare:
        return "DontCare";
    }
    return "Unknown";
}

const char* RenderResourceStateToString(RenderResourceState state) {
    switch (state) {
    case RenderResourceState::Undefined:
        return "Undefined";
    case RenderResourceState::ColorAttachment:
        return "ColorAttachment";
    case RenderResourceState::DepthStencilAttachment:
        return "DepthStencilAttachment";
    case RenderResourceState::ShaderRead:
        return "ShaderRead";
    case RenderResourceState::TransferSrc:
        return "TransferSrc";
    case RenderResourceState::TransferDst:
        return "TransferDst";
    case RenderResourceState::Present:
        return "Present";
    }
    return "Unknown";
}

TextureDesc MakeColorTargetTextureDesc(const RenderTargetDesc& desc) {
    TextureDesc out;
    out.width = desc.width;
    out.height = desc.height;
    out.mipLevels = 1;
    out.format = desc.colorFormat;
    out.usageFlags = TextureUsage_RenderTarget | TextureUsage_Sampled | TextureUsage_TransferSrc;
    out.debugName = desc.debugName.empty() ? std::string("RenderTargetColor") : desc.debugName + ".Color";
    return out;
}

TextureDesc MakeDepthTargetTextureDesc(const RenderTargetDesc& desc) {
    TextureDesc out;
    out.width = desc.width;
    out.height = desc.height;
    out.mipLevels = 1;
    out.format = TextureFormat::Depth32F;
    out.usageFlags = TextureUsage_DepthStencil | TextureUsage_Sampled;
    out.debugName = desc.debugName.empty() ? std::string("RenderTargetDepth") : desc.debugName + ".Depth";
    return out;
}

} // namespace Hockey

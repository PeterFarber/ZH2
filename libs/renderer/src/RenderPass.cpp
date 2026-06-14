#include "Hockey/Renderer/RenderPass.hpp"

namespace Hockey {

const char* RenderPassTypeToString(RenderPassType type) {
    switch (type) {
    case RenderPassType::Graphics:
        return "Graphics";
    case RenderPassType::Compute:
        return "Compute";
    case RenderPassType::Transfer:
        return "Transfer";
    }
    return "Unknown";
}

} // namespace Hockey

#include "Hockey/Renderer/Pipeline.hpp"

namespace Hockey {

const char* BuiltInPipelineName(BuiltInPipeline pipeline) {
    switch (pipeline) {
    case BuiltInPipeline::PbrOpaque:
        return "PbrOpaque";
    case BuiltInPipeline::DepthPrepass:
        return "DepthPrepass";
    case BuiltInPipeline::Shadow:
        return "Shadow";
    case BuiltInPipeline::Transparent:
        return "Transparent";
    case BuiltInPipeline::PostProcess:
        return "PostProcess";
    case BuiltInPipeline::DebugLine:
        return "DebugLine";
    case BuiltInPipeline::DebugShape:
        return "DebugShape";
    }
    return "Unknown";
}

} // namespace Hockey

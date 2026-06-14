#pragma once

namespace Hockey {

// The fixed set of pipelines the renderer builds. Backend-agnostic identifier
// used by the render path to look up a concrete VkPipeline.
enum class BuiltInPipeline {
    PbrOpaque,
    DepthPrepass,
    Shadow,
    Transparent,
    PostProcess,
    DebugLine,
    DebugShape,
};

const char* BuiltInPipelineName(BuiltInPipeline pipeline);

} // namespace Hockey

#pragma once

#include <cstdint>

namespace Hockey {

// Per-frame renderer statistics. Counters are reset at BeginFrame and
// accumulated while building/executing the frame.
struct RendererStats {
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t meshCount = 0;
    uint32_t materialCount = 0;
    uint32_t textureCount = 0;

    float cpuFrameMs = 0.0f;
    float gpuFrameMs = 0.0f;

    uint64_t usedVRAMBytes = 0;
    uint64_t budgetVRAMBytes = 0;

    uint32_t shadowDrawCalls = 0;
    uint32_t postProcessPasses = 0;

    // Reset per-frame counters; persistent totals (mesh/material/texture/VRAM)
    // are refreshed by the renderer separately.
    void ResetPerFrame() {
        drawCalls = 0;
        triangleCount = 0;
        shadowDrawCalls = 0;
        postProcessPasses = 0;
    }
};

} // namespace Hockey

#pragma once

#include <cstdint>

#include "Hockey/Renderer/Mesh.hpp"
#include "Hockey/Renderer/RenderDevice.hpp"
#include "Hockey/Renderer/Vulkan/VulkanBuffer.hpp"
#include "Hockey/Renderer/Vulkan/VulkanCommand.hpp"

namespace Hockey {

// A GPU mesh: device-local vertex + index buffers plus draw counts and bounds.
struct VulkanMesh {
    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    bool IsValid() const {
        return vertexBuffer.IsValid() && indexBuffer.IsValid() && indexCount > 0;
    }
};

VulkanMesh CreateMesh(const RenderDevice& device, const VulkanCommand& commands, const MeshDesc& desc);
void DestroyMesh(const RenderDevice& device, VulkanMesh& mesh);

} // namespace Hockey

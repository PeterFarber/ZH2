#include "Hockey/Renderer/Vulkan/VulkanMesh.hpp"

#include <limits>

namespace Hockey {

VulkanMesh CreateMesh(const RenderDevice& device, const VulkanCommand& commands, const MeshDesc& desc) {
    VulkanMesh mesh;
    if (desc.vertices.empty() || desc.indices.empty()) {
        HK_CORE_ERROR("CreateMesh: empty vertices or indices");
        return mesh;
    }
    mesh.vertexCount = static_cast<uint32_t>(desc.vertices.size());
    mesh.indexCount = static_cast<uint32_t>(desc.indices.size());
    mesh.boundsMin = glm::vec3(std::numeric_limits<float>::max());
    mesh.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const Vertex& vertex : desc.vertices) {
        mesh.boundsMin = glm::min(mesh.boundsMin, vertex.position);
        mesh.boundsMax = glm::max(mesh.boundsMax, vertex.position);
    }

    const size_t vertexBytes = desc.vertices.size() * sizeof(Vertex);
    const size_t indexBytes = desc.indices.size() * sizeof(uint32_t);

    BufferDesc vbDesc;
    vbDesc.size = vertexBytes;
    vbDesc.usage = BufferUsage::Vertex;
    vbDesc.cpuVisible = false; // device-local, staged
    vbDesc.debugName = desc.debugName.empty() ? "MeshVertexBuffer" : desc.debugName + ".vb";
    mesh.vertexBuffer = CreateBuffer(device, vbDesc);

    BufferDesc ibDesc;
    ibDesc.size = indexBytes;
    ibDesc.usage = BufferUsage::Index;
    ibDesc.cpuVisible = false;
    ibDesc.debugName = desc.debugName.empty() ? "MeshIndexBuffer" : desc.debugName + ".ib";
    mesh.indexBuffer = CreateBuffer(device, ibDesc);

    if (!mesh.vertexBuffer.IsValid() || !mesh.indexBuffer.IsValid()) {
        DestroyMesh(device, mesh);
        return {};
    }

    UploadBuffer(device, commands, mesh.vertexBuffer, desc.vertices.data(), vertexBytes);
    UploadBuffer(device, commands, mesh.indexBuffer, desc.indices.data(), indexBytes);
    return mesh;
}

void DestroyMesh(const RenderDevice& device, VulkanMesh& mesh) {
    DestroyBuffer(device, mesh.vertexBuffer);
    DestroyBuffer(device, mesh.indexBuffer);
    mesh.vertexCount = 0;
    mesh.indexCount = 0;
}

} // namespace Hockey

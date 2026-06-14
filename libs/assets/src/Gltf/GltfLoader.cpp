#include "Gltf/GltfLoader.hpp"

#include <algorithm>
#include <limits>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

std::string AlphaModeToString(fastgltf::AlphaMode mode) {
    switch (mode) {
    case fastgltf::AlphaMode::Mask:
        return "Mask";
    case fastgltf::AlphaMode::Blend:
        return "Blend";
    case fastgltf::AlphaMode::Opaque:
    default:
        return "Opaque";
    }
}

// Returns the authored URI of the image backing a texture, or "" when the
// texture is embedded / has no external file.
std::string TextureUri(const fastgltf::Asset& asset, const fastgltf::TextureInfo& info) {
    if (info.textureIndex >= asset.textures.size()) {
        return {};
    }
    const fastgltf::Texture& texture = asset.textures[info.textureIndex];
    if (!texture.imageIndex.has_value() || *texture.imageIndex >= asset.images.size()) {
        return {};
    }
    const fastgltf::Image& image = asset.images[*texture.imageIndex];
    std::string uri;
    std::visit(fastgltf::visitor{
                   [&](const fastgltf::sources::URI& fileSource) { uri = std::string(fileSource.uri.path()); },
                   [&](const auto&) {},
               },
               image.data);
    return uri;
}

GltfMaterialData ConvertMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material, size_t index) {
    GltfMaterialData out;
    out.name = material.name.empty() ? ("material_" + std::to_string(index)) : std::string(material.name);

    const auto& pbr = material.pbrData;
    out.baseColor = {pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]};
    out.metallic = pbr.metallicFactor;
    out.roughness = pbr.roughnessFactor;
    out.emissiveColor = {material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2]};
    out.emissiveStrength = material.emissiveStrength;
    out.alphaMode = AlphaModeToString(material.alphaMode);
    out.alphaCutoff = material.alphaCutoff;

    if (pbr.baseColorTexture.has_value()) {
        out.baseColorTexture = TextureUri(asset, *pbr.baseColorTexture);
    }
    if (pbr.metallicRoughnessTexture.has_value()) {
        out.metallicRoughnessTexture = TextureUri(asset, *pbr.metallicRoughnessTexture);
    }
    if (material.normalTexture.has_value()) {
        out.normalTexture = TextureUri(asset, *material.normalTexture);
        out.normalStrength = material.normalTexture->scale;
    }
    if (material.occlusionTexture.has_value()) {
        out.occlusionTexture = TextureUri(asset, *material.occlusionTexture);
        out.occlusionStrength = material.occlusionTexture->strength;
    }
    if (material.emissiveTexture.has_value()) {
        out.emissiveTexture = TextureUri(asset, *material.emissiveTexture);
    }
    return out;
}

// Appends one glTF primitive to the mesh-level buffers as a single submesh.
bool AppendPrimitive(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, GltfMeshData& mesh,
                     std::string& error) {
    const auto* positionIt = primitive.findAttribute("POSITION");
    if (positionIt == primitive.attributes.end()) {
        error = "glTF primitive missing POSITION attribute";
        return false;
    }

    const fastgltf::Accessor& positionAccessor = asset.accessors[positionIt->accessorIndex];
    const size_t baseVertex = mesh.vertices.size();
    const size_t vertexCount = positionAccessor.count;
    mesh.vertices.resize(baseVertex + vertexCount);

    fastgltf::iterateAccessorWithIndex<glm::vec3>(asset, positionAccessor, [&](glm::vec3 value, size_t i) {
        mesh.vertices[baseVertex + i].position = value;
        mesh.boundsMin = glm::min(mesh.boundsMin, value);
        mesh.boundsMax = glm::max(mesh.boundsMax, value);
    });

    if (const auto* it = primitive.findAttribute("NORMAL"); it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec3 value, size_t i) { mesh.vertices[baseVertex + i].normal = value; });
    }
    if (const auto* it = primitive.findAttribute("TANGENT"); it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec4 value, size_t i) { mesh.vertices[baseVertex + i].tangent = value; });
    }
    if (const auto* it = primitive.findAttribute("TEXCOORD_0"); it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec2 value, size_t i) { mesh.vertices[baseVertex + i].uv0 = value; });
    }

    MeshSubmesh submesh;
    submesh.firstIndex = static_cast<uint32_t>(mesh.indices.size());

    if (primitive.indicesAccessor.has_value()) {
        const fastgltf::Accessor& indexAccessor = asset.accessors[*primitive.indicesAccessor];
        mesh.indices.reserve(mesh.indices.size() + indexAccessor.count);
        fastgltf::iterateAccessor<uint32_t>(asset, indexAccessor, [&](uint32_t index) {
            mesh.indices.push_back(static_cast<uint32_t>(baseVertex) + index);
        });
        submesh.indexCount = static_cast<uint32_t>(indexAccessor.count);
    } else {
        // Non-indexed primitive: synthesize a sequential index buffer.
        for (size_t i = 0; i < vertexCount; ++i) {
            mesh.indices.push_back(static_cast<uint32_t>(baseVertex + i));
        }
        submesh.indexCount = static_cast<uint32_t>(vertexCount);
    }

    mesh.submeshes.push_back(submesh);
    mesh.submeshMaterialIndex.push_back(primitive.materialIndex.has_value() ? static_cast<int>(*primitive.materialIndex)
                                                                            : -1);
    return true;
}

} // namespace

Result<GltfScene> GltfLoader::Load(const fs::path& gltfPath) {
    auto dataResult = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (dataResult.error() != fastgltf::Error::None) {
        return Result<GltfScene>::Fail("cannot read glTF file: " + gltfPath.string());
    }

    constexpr auto options = fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;
    fastgltf::Parser parser;
    auto assetResult = parser.loadGltf(dataResult.get(), gltfPath.parent_path(), options);
    if (assetResult.error() != fastgltf::Error::None) {
        return Result<GltfScene>::Fail("failed to parse glTF '" + gltfPath.string() +
                                       "': " + std::string(fastgltf::getErrorMessage(assetResult.error())));
    }
    const fastgltf::Asset& asset = assetResult.get();

    GltfScene scene;
    scene.materials.reserve(asset.materials.size());
    for (size_t i = 0; i < asset.materials.size(); ++i) {
        scene.materials.push_back(ConvertMaterial(asset, asset.materials[i], i));
    }

    scene.meshes.reserve(asset.meshes.size());
    for (size_t meshIndex = 0; meshIndex < asset.meshes.size(); ++meshIndex) {
        const fastgltf::Mesh& gltfMesh = asset.meshes[meshIndex];
        GltfMeshData mesh;
        mesh.name = gltfMesh.name.empty() ? ("mesh_" + std::to_string(meshIndex)) : std::string(gltfMesh.name);
        mesh.boundsMin = glm::vec3(std::numeric_limits<float>::max());
        mesh.boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

        for (const fastgltf::Primitive& primitive : gltfMesh.primitives) {
            std::string error;
            if (!AppendPrimitive(asset, primitive, mesh, error)) {
                return Result<GltfScene>::Fail(error);
            }
        }

        if (mesh.vertices.empty()) {
            mesh.boundsMin = glm::vec3(0.0f);
            mesh.boundsMax = glm::vec3(0.0f);
        }
        scene.meshes.push_back(std::move(mesh));
    }

    return Result<GltfScene>::Ok(std::move(scene));
}

} // namespace Hockey

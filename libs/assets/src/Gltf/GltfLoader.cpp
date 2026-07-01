#include "Gltf/GltfLoader.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

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

// File extension matching a glTF image mime type, or "" when unknown.
std::string MimeExtension(fastgltf::MimeType mime) {
    switch (mime) {
    case fastgltf::MimeType::JPEG:
        return "jpg";
    case fastgltf::MimeType::PNG:
        return "png";
    case fastgltf::MimeType::KTX2:
        return "ktx2";
    case fastgltf::MimeType::DDS:
        return "dds";
    case fastgltf::MimeType::WEBP:
        return "webp";
    default:
        return {};
    }
}

struct EncodedImage {
    std::vector<std::byte> bytes;
    std::string extension;
};

template <typename Container>
void CopyRange(std::vector<std::byte>& out, const Container& src, size_t offset, size_t length) {
    if (offset + length > src.size()) {
        return;
    }
    out.assign(src.data() + offset, src.data() + offset + length);
}

// Pulls the encoded image bytes for an embedded glTF image (buffer-view source
// in a GLB, or an inline data-URI decoded into memory). Returns nullopt for
// external (file URI) images and for sources with an unknown mime type.
std::optional<EncodedImage> ExtractEmbeddedImage(const fastgltf::Asset& asset, size_t imageIndex) {
    const fastgltf::Image& image = asset.images[imageIndex];
    EncodedImage result;
    bool ok = false;

    std::visit(fastgltf::visitor{
                   [&](const fastgltf::sources::BufferView& src) {
                       if (src.bufferViewIndex >= asset.bufferViews.size()) {
                           return;
                       }
                       const fastgltf::BufferView& view = asset.bufferViews[src.bufferViewIndex];
                       if (view.bufferIndex >= asset.buffers.size()) {
                           return;
                       }
                       result.extension = MimeExtension(src.mimeType);
                       const fastgltf::Buffer& buffer = asset.buffers[view.bufferIndex];
                       std::visit(fastgltf::visitor{
                                      [&](const fastgltf::sources::Array& a) {
                                          CopyRange(result.bytes, a.bytes, view.byteOffset, view.byteLength);
                                          ok = !result.bytes.empty();
                                      },
                                      [&](const fastgltf::sources::Vector& v) {
                                          CopyRange(result.bytes, v.bytes, view.byteOffset, view.byteLength);
                                          ok = !result.bytes.empty();
                                      },
                                      [&](const fastgltf::sources::ByteView& b) {
                                          CopyRange(result.bytes, b.bytes, view.byteOffset, view.byteLength);
                                          ok = !result.bytes.empty();
                                      },
                                      [&](const auto&) {},
                                  },
                                  buffer.data);
                   },
                   [&](const fastgltf::sources::Array& a) {
                       result.extension = MimeExtension(a.mimeType);
                       CopyRange(result.bytes, a.bytes, 0, a.bytes.size());
                       ok = !result.bytes.empty();
                   },
                   [&](const fastgltf::sources::Vector& v) {
                       result.extension = MimeExtension(v.mimeType);
                       CopyRange(result.bytes, v.bytes, 0, v.bytes.size());
                       ok = !result.bytes.empty();
                   },
                   [&](const fastgltf::sources::ByteView& b) {
                       result.extension = MimeExtension(b.mimeType);
                       CopyRange(result.bytes, b.bytes, 0, b.bytes.size());
                       ok = !result.bytes.empty();
                   },
                   [&](const auto&) {},
               },
               image.data);

    if (!ok || result.extension.empty()) {
        return std::nullopt;
    }
    return result;
}

// Carries the per-scene state needed to turn glTF texture references into
// project-relative paths, externalizing embedded images on first use.
struct TextureResolver {
    const fastgltf::Asset& asset;
    std::string textureDir;                               // e.g. "skater.textures/"
    std::vector<GltfEmbeddedTexture>& embedded;
    std::unordered_map<size_t, std::string> imageCache;   // image index -> relative path

    // Returns the path (relative to the glTF directory) for a texture slot.
    // External images keep their authored URI; embedded images are written to a
    // deterministic sibling path whose name encodes the role so the importer's
    // color-space inference picks sRGB/linear correctly.
    std::string Resolve(const fastgltf::TextureInfo& info, const char* role) {
        if (info.textureIndex >= asset.textures.size()) {
            return {};
        }
        const fastgltf::Texture& texture = asset.textures[info.textureIndex];
        if (!texture.imageIndex.has_value() || *texture.imageIndex >= asset.images.size()) {
            return {};
        }
        const size_t imageIndex = *texture.imageIndex;
        const fastgltf::Image& image = asset.images[imageIndex];

        if (const auto* fileSource = std::get_if<fastgltf::sources::URI>(&image.data)) {
            return std::string(fileSource->uri.path());
        }

        if (const auto it = imageCache.find(imageIndex); it != imageCache.end()) {
            return it->second;
        }
        std::optional<EncodedImage> encoded = ExtractEmbeddedImage(asset, imageIndex);
        if (!encoded.has_value()) {
            return {};
        }
        std::string relativePath =
            textureDir + "img" + std::to_string(imageIndex) + "_" + role + "." + encoded->extension;
        embedded.push_back(GltfEmbeddedTexture{relativePath, std::move(encoded->bytes)});
        imageCache.emplace(imageIndex, relativePath);
        return relativePath;
    }
};

GltfMaterialData ConvertMaterial(const fastgltf::Material& material, size_t index, TextureResolver& textures) {
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
        out.baseColorTexture = textures.Resolve(*pbr.baseColorTexture, "basecolor");
    }
    if (pbr.metallicRoughnessTexture.has_value()) {
        out.metallicRoughnessTexture = textures.Resolve(*pbr.metallicRoughnessTexture, "metallicroughness");
    }
    if (material.normalTexture.has_value()) {
        out.normalTexture = textures.Resolve(*material.normalTexture, "normal");
        out.normalStrength = material.normalTexture->scale;
    }
    if (material.occlusionTexture.has_value()) {
        out.occlusionTexture = textures.Resolve(*material.occlusionTexture, "occlusion");
        out.occlusionStrength = material.occlusionTexture->strength;
    }
    if (material.emissiveTexture.has_value()) {
        out.emissiveTexture = textures.Resolve(*material.emissiveTexture, "emissive");
    }
    return out;
}

// Computes per-vertex tangents (Lengyel's method) for a freshly appended
// submesh whose source primitive omitted the TANGENT attribute. Requires
// positions, normals and UVs (all present on the appended vertices). The w
// component stores the bitangent handedness, matching the glTF convention.
void GenerateTangents(GltfMeshData& mesh, size_t baseVertex, size_t vertexCount, uint32_t firstIndex,
                      uint32_t indexCount) {
    if (vertexCount == 0 || indexCount < 3) {
        return;
    }
    std::vector<glm::vec3> tan1(vertexCount, glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(vertexCount, glm::vec3(0.0f));

    for (uint32_t t = 0; t + 2 < indexCount; t += 3) {
        const size_t i0 = mesh.indices[firstIndex + t] - baseVertex;
        const size_t i1 = mesh.indices[firstIndex + t + 1] - baseVertex;
        const size_t i2 = mesh.indices[firstIndex + t + 2] - baseVertex;
        if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) {
            continue;
        }
        const MeshVertex& v0 = mesh.vertices[baseVertex + i0];
        const MeshVertex& v1 = mesh.vertices[baseVertex + i1];
        const MeshVertex& v2 = mesh.vertices[baseVertex + i2];

        const glm::vec3 e1 = v1.position - v0.position;
        const glm::vec3 e2 = v2.position - v0.position;
        const glm::vec2 duv1 = v1.uv0 - v0.uv0;
        const glm::vec2 duv2 = v2.uv0 - v0.uv0;

        const float denom = duv1.x * duv2.y - duv2.x * duv1.y;
        const float r = (std::abs(denom) < 1e-8f) ? 0.0f : 1.0f / denom;
        const glm::vec3 sdir = (e1 * duv2.y - e2 * duv1.y) * r;
        const glm::vec3 tdir = (e2 * duv1.x - e1 * duv2.x) * r;

        tan1[i0] += sdir;
        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan2[i0] += tdir;
        tan2[i1] += tdir;
        tan2[i2] += tdir;
    }

    for (size_t i = 0; i < vertexCount; ++i) {
        const glm::vec3 n = mesh.vertices[baseVertex + i].normal;
        const glm::vec3 t = tan1[i];
        const glm::vec3 ortho = t - n * glm::dot(n, t);
        const float length = glm::length(ortho);
        const glm::vec3 tangent = length > 1e-8f ? ortho / length : glm::vec3(1.0f, 0.0f, 0.0f);
        const float handedness = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0f) ? -1.0f : 1.0f;
        mesh.vertices[baseVertex + i].tangent = glm::vec4(tangent, handedness);
    }
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
    bool hasTangents = false;
    if (const auto* it = primitive.findAttribute("TANGENT"); it != primitive.attributes.end()) {
        hasTangents = true;
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec4 value, size_t i) { mesh.vertices[baseVertex + i].tangent = value; });
    }
    bool hasUVs = false;
    if (const auto* it = primitive.findAttribute("TEXCOORD_0"); it != primitive.attributes.end()) {
        hasUVs = true;
        fastgltf::iterateAccessorWithIndex<glm::vec2>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec2 value, size_t i) { mesh.vertices[baseVertex + i].uv0 = value; });
    }
    if (const auto* it = primitive.findAttribute("JOINTS_0"); it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::u16vec4>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::u16vec4 value, size_t i) { mesh.vertices[baseVertex + i].jointIndices = value; });
    }
    if (const auto* it = primitive.findAttribute("WEIGHTS_0"); it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec4>(
            asset, asset.accessors[it->accessorIndex],
            [&](glm::vec4 value, size_t i) { mesh.vertices[baseVertex + i].jointWeights = value; });
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

    // glTF allows omitting tangents; the renderer's normal mapping needs them,
    // so synthesize them from positions/UVs when the source lacked TANGENT.
    if (!hasTangents && hasUVs) {
        GenerateTangents(mesh, baseVertex, vertexCount, submesh.firstIndex, submesh.indexCount);
    }

    mesh.submeshes.push_back(submesh);
    mesh.submeshMaterialIndex.push_back(primitive.materialIndex.has_value() ? static_cast<int>(*primitive.materialIndex)
                                                                            : -1);
    return true;
}

glm::mat4 ToGlm(const fastgltf::math::fmat4x4& value) {
    glm::mat4 out{1.0f};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            out[column][row] = value[column][row];
        }
    }
    return out;
}

std::vector<size_t> BuildParentNodes(const fastgltf::Asset& asset) {
    constexpr size_t kNoParent = std::numeric_limits<size_t>::max();
    std::vector<size_t> parents(asset.nodes.size(), kNoParent);
    for (size_t parentIndex = 0; parentIndex < asset.nodes.size(); ++parentIndex) {
        for (const size_t childIndex : asset.nodes[parentIndex].children) {
            if (childIndex < parents.size()) {
                parents[childIndex] = parentIndex;
            }
        }
    }
    return parents;
}

std::vector<glm::mat4> ReadInverseBindMatrices(const fastgltf::Asset& asset, const fastgltf::Skin& skin) {
    std::vector<glm::mat4> matrices(skin.joints.size(), glm::mat4{1.0f});
    if (!skin.inverseBindMatrices.has_value() || *skin.inverseBindMatrices >= asset.accessors.size()) {
        return matrices;
    }
    fastgltf::iterateAccessorWithIndex<glm::mat4>(
        asset, asset.accessors[*skin.inverseBindMatrices],
        [&](const glm::mat4& value, size_t index) {
            if (index < matrices.size()) {
                matrices[index] = value;
            }
        });
    return matrices;
}

std::vector<std::unordered_map<size_t, int>> ExtractSkeletons(const fastgltf::Asset& asset,
                                                              const std::vector<size_t>& parentNodes,
                                                              GltfScene& scene) {
    constexpr size_t kNoParent = std::numeric_limits<size_t>::max();
    std::vector<std::unordered_map<size_t, int>> jointToBoneBySkeleton;
    jointToBoneBySkeleton.reserve(asset.skins.size());
    scene.skeletons.reserve(asset.skins.size());

    for (size_t skinIndex = 0; skinIndex < asset.skins.size(); ++skinIndex) {
        const fastgltf::Skin& skin = asset.skins[skinIndex];
        GltfSkeletonData skeleton;
        skeleton.name = skin.name.empty() ? ("skeleton_" + std::to_string(skinIndex)) : std::string(skin.name);
        skeleton.jointNodeIndices.assign(skin.joints.begin(), skin.joints.end());

        std::unordered_map<size_t, int> jointToBone;
        for (size_t boneIndex = 0; boneIndex < skin.joints.size(); ++boneIndex) {
            jointToBone.emplace(skin.joints[boneIndex], static_cast<int>(boneIndex));
        }

        const std::vector<glm::mat4> inverseBindMatrices = ReadInverseBindMatrices(asset, skin);
        skeleton.bones.reserve(skin.joints.size());
        for (size_t boneIndex = 0; boneIndex < skin.joints.size(); ++boneIndex) {
            const size_t nodeIndex = skin.joints[boneIndex];
            SkeletonAssetBone bone;
            bone.name = nodeIndex < asset.nodes.size() && !asset.nodes[nodeIndex].name.empty()
                            ? std::string(asset.nodes[nodeIndex].name)
                            : ("joint_" + std::to_string(boneIndex));
            if (nodeIndex < parentNodes.size() && parentNodes[nodeIndex] != kNoParent) {
                if (const auto foundParent = jointToBone.find(parentNodes[nodeIndex]); foundParent != jointToBone.end()) {
                    bone.parentIndex = foundParent->second;
                }
            }
            if (boneIndex < inverseBindMatrices.size()) {
                bone.inverseBindPose = inverseBindMatrices[boneIndex];
            }
            if (nodeIndex < asset.nodes.size()) {
                bone.localBindTransform = ToGlm(fastgltf::getTransformMatrix(asset.nodes[nodeIndex]));
            }
            skeleton.bones.push_back(std::move(bone));
        }

        jointToBoneBySkeleton.push_back(std::move(jointToBone));
        scene.skeletons.push_back(std::move(skeleton));
    }

    return jointToBoneBySkeleton;
}

void AssignMeshSkins(const fastgltf::Asset& asset, GltfScene& scene) {
    for (const fastgltf::Node& node : asset.nodes) {
        if (!node.meshIndex.has_value() || !node.skinIndex.has_value()) {
            continue;
        }
        if (*node.meshIndex >= scene.meshes.size() || *node.skinIndex >= scene.skeletons.size()) {
            continue;
        }
        if (scene.meshes[*node.meshIndex].skinIndex < 0) {
            scene.meshes[*node.meshIndex].skinIndex = static_cast<int>(*node.skinIndex);
        }
    }
}

std::vector<float> ReadAnimationTimes(const fastgltf::Asset& asset, size_t accessorIndex) {
    std::vector<float> times;
    if (accessorIndex >= asset.accessors.size()) {
        return times;
    }
    const fastgltf::Accessor& accessor = asset.accessors[accessorIndex];
    times.reserve(accessor.count);
    fastgltf::iterateAccessor<float>(asset, accessor, [&](float value) { times.push_back(value); });
    return times;
}

template <typename T> std::vector<T> ReadAnimationValues(const fastgltf::Asset& asset, size_t accessorIndex) {
    std::vector<T> values;
    if (accessorIndex >= asset.accessors.size()) {
        return values;
    }
    const fastgltf::Accessor& accessor = asset.accessors[accessorIndex];
    values.reserve(accessor.count);
    fastgltf::iterateAccessor<T>(asset, accessor, [&](T value) { values.push_back(value); });
    return values;
}

AnimationBoneTrack& FindOrAddTrack(std::vector<AnimationBoneTrack>& tracks, int boneIndex) {
    for (AnimationBoneTrack& track : tracks) {
        if (track.boneIndex == boneIndex) {
            return track;
        }
    }
    AnimationBoneTrack track;
    track.boneIndex = boneIndex;
    tracks.push_back(std::move(track));
    return tracks.back();
}

bool FindSkeletonBoneForNode(const std::vector<std::unordered_map<size_t, int>>& jointToBoneBySkeleton,
                             size_t nodeIndex, int& skeletonIndex, int& boneIndex) {
    for (size_t i = 0; i < jointToBoneBySkeleton.size(); ++i) {
        if (const auto found = jointToBoneBySkeleton[i].find(nodeIndex); found != jointToBoneBySkeleton[i].end()) {
            skeletonIndex = static_cast<int>(i);
            boneIndex = found->second;
            return true;
        }
    }
    return false;
}

size_t AnimationValueIndex(const fastgltf::AnimationSampler& sampler, size_t keyIndex) {
    if (sampler.interpolation == fastgltf::AnimationInterpolation::CubicSpline) {
        return keyIndex * 3 + 1;
    }
    return keyIndex;
}

void ExtractAnimations(const fastgltf::Asset& asset,
                       const std::vector<std::unordered_map<size_t, int>>& jointToBoneBySkeleton,
                       GltfScene& scene) {
    scene.animations.reserve(asset.animations.size());
    for (size_t animationIndex = 0; animationIndex < asset.animations.size(); ++animationIndex) {
        const fastgltf::Animation& gltfAnimation = asset.animations[animationIndex];
        GltfAnimationData animation;
        animation.name =
            gltfAnimation.name.empty() ? ("animation_" + std::to_string(animationIndex)) : std::string(gltfAnimation.name);

        for (const fastgltf::AnimationChannel& channel : gltfAnimation.channels) {
            if (!channel.nodeIndex.has_value() || channel.samplerIndex >= gltfAnimation.samplers.size()) {
                continue;
            }

            int channelSkeleton = -1;
            int boneIndex = -1;
            if (!FindSkeletonBoneForNode(jointToBoneBySkeleton, *channel.nodeIndex, channelSkeleton, boneIndex)) {
                continue;
            }
            if (animation.skeletonIndex < 0) {
                animation.skeletonIndex = channelSkeleton;
            }
            if (animation.skeletonIndex != channelSkeleton) {
                continue;
            }

            const fastgltf::AnimationSampler& sampler = gltfAnimation.samplers[channel.samplerIndex];
            const std::vector<float> times = ReadAnimationTimes(asset, sampler.inputAccessor);
            if (times.empty()) {
                continue;
            }
            animation.durationSeconds = std::max(animation.durationSeconds, times.back());
            AnimationBoneTrack& track = FindOrAddTrack(animation.tracks, boneIndex);

            if (channel.path == fastgltf::AnimationPath::Translation) {
                const std::vector<glm::vec3> values = ReadAnimationValues<glm::vec3>(asset, sampler.outputAccessor);
                for (size_t i = 0; i < times.size(); ++i) {
                    const size_t valueIndex = AnimationValueIndex(sampler, i);
                    if (valueIndex < values.size()) {
                        track.translations.push_back({times[i], values[valueIndex]});
                    }
                }
            } else if (channel.path == fastgltf::AnimationPath::Rotation) {
                const std::vector<glm::vec4> values = ReadAnimationValues<glm::vec4>(asset, sampler.outputAccessor);
                for (size_t i = 0; i < times.size(); ++i) {
                    const size_t valueIndex = AnimationValueIndex(sampler, i);
                    if (valueIndex < values.size()) {
                        const glm::vec4 value = values[valueIndex];
                        track.rotations.push_back({times[i], glm::quat(value.w, value.x, value.y, value.z)});
                    }
                }
            } else if (channel.path == fastgltf::AnimationPath::Scale) {
                const std::vector<glm::vec3> values = ReadAnimationValues<glm::vec3>(asset, sampler.outputAccessor);
                for (size_t i = 0; i < times.size(); ++i) {
                    const size_t valueIndex = AnimationValueIndex(sampler, i);
                    if (valueIndex < values.size()) {
                        track.scales.push_back({times[i], values[valueIndex]});
                    }
                }
            }
        }

        if (animation.skeletonIndex >= 0 && !animation.tracks.empty()) {
            scene.animations.push_back(std::move(animation));
        }
    }
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
    // Embedded images are externalized next to the model as "<stem>.textures/...".
    TextureResolver textures{asset, gltfPath.stem().string() + ".textures/", scene.embeddedTextures, {}};
    scene.materials.reserve(asset.materials.size());
    for (size_t i = 0; i < asset.materials.size(); ++i) {
        scene.materials.push_back(ConvertMaterial(asset.materials[i], i, textures));
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

    const std::vector<size_t> parentNodes = BuildParentNodes(asset);
    const std::vector<std::unordered_map<size_t, int>> jointToBoneBySkeleton =
        ExtractSkeletons(asset, parentNodes, scene);
    AssignMeshSkins(asset, scene);
    ExtractAnimations(asset, jointToBoneBySkeleton, scene);

    return Result<GltfScene>::Ok(std::move(scene));
}

} // namespace Hockey

#pragma once

#include <cstdint>
#include <functional>

namespace Hockey {

// Opaque, type-safe handles to GPU resources owned by the Renderer.
// id == 0 always means "invalid / null". Handles are values; the renderer
// maps them back to internal Vulkan objects. Gameplay/editor code must never
// see raw Vulkan handles.

#define HK_DEFINE_RENDER_HANDLE(Name)                                                                                  \
    struct Name {                                                                                                      \
        uint32_t id = 0;                                                                                               \
        bool IsValid() const {                                                                                         \
            return id != 0;                                                                                            \
        }                                                                                                              \
        bool operator==(const Name& other) const {                                                                     \
            return id == other.id;                                                                                     \
        }                                                                                                              \
        bool operator!=(const Name& other) const {                                                                     \
            return id != other.id;                                                                                     \
        }                                                                                                              \
    }

HK_DEFINE_RENDER_HANDLE(MeshHandle);
HK_DEFINE_RENDER_HANDLE(TextureHandle);
HK_DEFINE_RENDER_HANDLE(MaterialHandle);
HK_DEFINE_RENDER_HANDLE(ShaderHandle);
HK_DEFINE_RENDER_HANDLE(PipelineHandle);
HK_DEFINE_RENDER_HANDLE(BufferHandle);
HK_DEFINE_RENDER_HANDLE(SamplerHandle);
HK_DEFINE_RENDER_HANDLE(RenderPassHandle);

#undef HK_DEFINE_RENDER_HANDLE

} // namespace Hockey

// Hash specializations so handles can be used as map keys.
#define HK_RENDER_HANDLE_HASH(Name)                                                                                    \
    template <> struct std::hash<Hockey::Name> {                                                                       \
        std::size_t operator()(const Hockey::Name& h) const noexcept {                                                 \
            return std::hash<uint32_t>{}(h.id);                                                                        \
        }                                                                                                              \
    }

HK_RENDER_HANDLE_HASH(MeshHandle);
HK_RENDER_HANDLE_HASH(TextureHandle);
HK_RENDER_HANDLE_HASH(MaterialHandle);
HK_RENDER_HANDLE_HASH(ShaderHandle);
HK_RENDER_HANDLE_HASH(PipelineHandle);
HK_RENDER_HANDLE_HASH(BufferHandle);
HK_RENDER_HANDLE_HASH(SamplerHandle);
HK_RENDER_HANDLE_HASH(RenderPassHandle);

#undef HK_RENDER_HANDLE_HASH

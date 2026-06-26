#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace Hockey {

#define HK_DEFINE_UI_OVERLAY_HANDLE(Name)                                                                              \
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

HK_DEFINE_UI_OVERLAY_HANDLE(UIOverlayGeometryHandle);
HK_DEFINE_UI_OVERLAY_HANDLE(UIOverlayTextureHandle);

#undef HK_DEFINE_UI_OVERLAY_HANDLE

struct UIOverlayVertex {
    glm::vec2 position{0.0f};
    glm::vec2 texCoord{0.0f};
    glm::vec4 color{1.0f};
};

struct UIOverlayGeometryDesc {
    std::vector<UIOverlayVertex> vertices;
    std::vector<uint32_t> indices;
    std::string debugName;
};

struct UIOverlayTextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    const void* rgba8Pixels = nullptr;
    std::size_t rgba8ByteCount = 0;
    std::string debugName;
};

struct UIOverlayScissor {
    int x = 0;
    int y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct UIOverlayDrawCommand {
    UIOverlayGeometryHandle geometry;
    UIOverlayTextureHandle texture;
    glm::vec2 translation{0.0f};
    std::optional<glm::mat4> transform;
    std::optional<UIOverlayScissor> scissor;
};

} // namespace Hockey

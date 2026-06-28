#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <imgui.h>

#include "Hockey/Renderer/RendererSettings.hpp"

namespace Hockey::EditorViewport {

struct Frame {
    ImVec2 imagePos{0.0f, 0.0f};
    ImVec2 imageSize{1.0f, 1.0f};
    std::uint32_t width = 1;
    std::uint32_t height = 1;
    float aspect = 16.0f / 9.0f;
};

inline float AspectFromSettings(const RendererSettings& settings) {
    const std::uint32_t width = std::max(1u, settings.resolutionWidth);
    const std::uint32_t height = std::max(1u, settings.resolutionHeight);
    return static_cast<float>(width) / static_cast<float>(height);
}

inline Frame ComputeFrame(const ImVec2& basePos, const ImVec2& available, const RendererSettings& settings) {
    const float aspect = AspectFromSettings(settings);
    const float availableWidth = std::max(1.0f, available.x);
    const float availableHeight = std::max(1.0f, available.y);

    float imageWidth = availableWidth;
    float imageHeight = imageWidth / aspect;
    if (imageHeight > availableHeight) {
        imageHeight = availableHeight;
        imageWidth = imageHeight * aspect;
    }

    imageWidth = std::max(1.0f, std::floor(imageWidth));
    imageHeight = std::max(1.0f, std::floor(imageHeight));

    const auto displayWidth = static_cast<std::uint32_t>(imageWidth);
    const std::uint32_t renderWidth = std::max(displayWidth, std::max(1u, settings.resolutionWidth));

    Frame frame;
    frame.imageSize = ImVec2(imageWidth, imageHeight);
    frame.imagePos =
        ImVec2(basePos.x + std::floor((availableWidth - imageWidth) * 0.5f),
               basePos.y + std::floor((availableHeight - imageHeight) * 0.5f));
    frame.width = renderWidth;
    frame.height =
        std::max(1u, static_cast<std::uint32_t>(std::floor(static_cast<float>(renderWidth) / aspect + 0.5f)));
    frame.aspect = aspect;
    return frame;
}

inline ImVec2 ImagePointToRenderPixels(const Frame& frame, const ImVec2& imagePoint) {
    const float imageWidth = std::max(1.0f, frame.imageSize.x);
    const float imageHeight = std::max(1.0f, frame.imageSize.y);
    const float clampedX = std::clamp(imagePoint.x, 0.0f, imageWidth);
    const float clampedY = std::clamp(imagePoint.y, 0.0f, imageHeight);
    const float scaleX = static_cast<float>(std::max(1u, frame.width)) / imageWidth;
    const float scaleY = static_cast<float>(std::max(1u, frame.height)) / imageHeight;
    return ImVec2(clampedX * scaleX, clampedY * scaleY);
}

inline bool Contains(const ImVec2& min, const ImVec2& size, const ImVec2& point) {
    return point.x >= min.x && point.x < min.x + size.x && point.y >= min.y && point.y < min.y + size.y;
}

} // namespace Hockey::EditorViewport

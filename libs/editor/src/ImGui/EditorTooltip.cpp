#include "Hockey/Editor/ImGui/EditorTooltip.hpp"

#include <algorithm>
#include <limits>

#include <imgui.h>

namespace Hockey::EditorTooltip {

ImGuiHoveredFlags DefaultHoverFlags() {
    return ImGuiHoveredFlags_DelayShort | ImGuiHoveredFlags_AllowWhenDisabled;
}

void ForLastItem(const char* text) {
    if (text == nullptr || text[0] == '\0') {
        return;
    }
    if (ImGui::IsItemHovered(DefaultHoverFlags())) {
        ImGui::SetTooltip("%s", text);
    }
}

void ForLastItem(std::string_view text) {
    if (text.empty()) {
        return;
    }
    if (ImGui::IsItemHovered(DefaultHoverFlags())) {
        const std::size_t cappedSize =
            std::min(text.size(), static_cast<std::size_t>(std::numeric_limits<int>::max()));
        ImGui::SetTooltip("%.*s", static_cast<int>(cappedSize), text.data());
    }
}

} // namespace Hockey::EditorTooltip

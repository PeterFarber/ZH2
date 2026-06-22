#pragma once

#include <string_view>

#include <imgui.h>

namespace Hockey::EditorTooltip {

ImGuiHoveredFlags DefaultHoverFlags();

void ForLastItem(const char* text);
void ForLastItem(std::string_view text);

} // namespace Hockey::EditorTooltip

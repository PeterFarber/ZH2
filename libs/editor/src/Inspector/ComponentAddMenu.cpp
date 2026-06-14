#include "Hockey/Editor/Inspector/ComponentAddMenu.hpp"

#include <cctype>
#include <string>

#include <imgui.h>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/Editor/EditorCommands.hpp"
#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

namespace {

bool MatchesFilter(const std::string& haystack, const char* needle) {
    if (needle == nullptr || needle[0] == '\0') {
        return true;
    }
    std::string lowerHay = haystack;
    std::string lowerNeedle = needle;
    const auto toLower = [](std::string& text) {
        for (char& ch : text) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
    };
    toLower(lowerHay);
    toLower(lowerNeedle);
    return lowerHay.find(lowerNeedle) != std::string::npos;
}

} // namespace

void ComponentAddMenu::Draw(EditorContext& context, Entity& entity) {
    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f))) {
        m_Search[0] = '\0';
        ImGui::OpenPopup("##AddComponentPopup");
    }

    if (!ImGui::BeginPopup("##AddComponentPopup")) {
        return;
    }

    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Search...", m_Search, sizeof(m_Search));
    ImGui::Separator();

    for (const ComponentMetadata& metadata : ComponentRegistry::Get().All()) {
        if (!metadata.addable) {
            continue;
        }
        if (metadata.has && metadata.has(entity)) {
            continue;
        }
        if (!MatchesFilter(metadata.displayName, m_Search)) {
            continue;
        }
        if (ImGui::Selectable(metadata.displayName.c_str())) {
            if (metadata.add) {
                context.undoRedo.Execute(EditorCommands::AddComponent(entity.GetUUID(), metadata.name), context);
            }
            ImGui::CloseCurrentPopup();
        }
    }

    ImGui::EndPopup();
}

} // namespace Hockey

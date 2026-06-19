#include "Hockey/Editor/Inspector/ComponentAddMenu.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

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

bool HasSearch(const char* search) {
    return search != nullptr && search[0] != '\0';
}

bool CanAdd(const ComponentMetadata& metadata, Entity& entity) {
    if (!metadata.addable) {
        return false;
    }
    if (metadata.has && metadata.has(entity)) {
        return false;
    }
    return true;
}

std::string SearchText(const ComponentMetadata& metadata) {
    return metadata.displayName + " " + metadata.name + " " + metadata.category;
}

void AddComponent(EditorContext& context, Entity& entity, const ComponentMetadata& metadata) {
    if (metadata.add) {
        context.undoRedo.Execute(EditorCommands::AddComponent(entity.GetUUID(), metadata.name), context);
    }
    ImGui::CloseCurrentPopup();
}

} // namespace

void ComponentAddMenu::Draw(EditorContext& context, Entity& entity) {
    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f))) {
        m_Search[0] = '\0';
        ImGui::OpenPopup("##AddComponentPopup");
    }

    const ImVec2 popupSize(320.0f, 420.0f);
    ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
    ImGui::SetNextWindowSizeConstraints(popupSize, popupSize);
    if (!ImGui::BeginPopup("##AddComponentPopup")) {
        return;
    }

    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##search", "Search...", m_Search, sizeof(m_Search));
    ImGui::Separator();
    ImGui::BeginChild("##componentList", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);

    std::vector<const ComponentMetadata*> available;
    for (const ComponentMetadata& metadata : ComponentRegistry::Get().All()) {
        if (CanAdd(metadata, entity)) {
            available.push_back(&metadata);
        }
    }
    std::sort(available.begin(), available.end(), [](const ComponentMetadata* lhs, const ComponentMetadata* rhs) {
        return lhs->displayName < rhs->displayName;
    });

    if (HasSearch(m_Search)) {
        bool drewAny = false;
        for (const ComponentMetadata* metadata : available) {
            if (!MatchesFilter(SearchText(*metadata), m_Search)) {
                continue;
            }
            drewAny = true;
            const std::string label = metadata->category + " / " + metadata->displayName;
            if (ImGui::Selectable(label.c_str())) {
                AddComponent(context, entity, *metadata);
            }
        }
        if (!drewAny) {
            ImGui::TextDisabled("No matching components.");
        }
    } else {
        constexpr const char* kCategories[] = {"Rendering", "Physics", "Hockey", "Gameplay", "Miscellaneous"};
        bool drewAny = false;
        for (const char* category : kCategories) {
            bool hasCategory = false;
            for (const ComponentMetadata* metadata : available) {
                if (metadata->category == category) {
                    hasCategory = true;
                    break;
                }
            }
            if (!hasCategory) {
                continue;
            }
            drewAny = true;
            if (ImGui::BeginMenu(category)) {
                for (const ComponentMetadata* metadata : available) {
                    if (metadata->category != category) {
                        continue;
                    }
                    if (ImGui::Selectable(metadata->displayName.c_str())) {
                        AddComponent(context, entity, *metadata);
                    }
                }
                ImGui::EndMenu();
            }
        }
        if (!drewAny) {
            ImGui::TextDisabled("No components available.");
        }
    }

    ImGui::EndChild();
    ImGui::EndPopup();
}

} // namespace Hockey

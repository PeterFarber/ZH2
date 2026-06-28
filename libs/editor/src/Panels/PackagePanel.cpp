#include "Hockey/Editor/Panels/PackagePanel.hpp"

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Editor/Dockspace.hpp"
#include "Hockey/Editor/EditorContext.hpp"
#include "Hockey/Editor/Packaging/PackageCommand.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include <imgui.h>

namespace Hockey {
namespace {

bool InputString(const char* label, std::string& value) {
    std::array<char, 256> buffer{};
    const std::size_t copy = std::min(value.size(), buffer.size() - 1);
    std::memcpy(buffer.data(), value.data(), copy);
    if (ImGui::InputText(label, buffer.data(), buffer.size())) {
        value = buffer.data();
        return true;
    }
    return false;
}

bool InputPath(const char* label, std::filesystem::path& value) {
    std::string text = value.generic_string();
    if (InputString(label, text)) {
        value = text;
        return true;
    }
    return false;
}

} // namespace

PackagePanel::PackagePanel() : Panel(EditorPanelNames::kPackage, /*openByDefault=*/false) {}

void PackagePanel::EnsureLoaded() {
    if (m_Loaded) {
        return;
    }
    const Result<PackageProfiles> loaded = LoadPackageProfiles();
    if (loaded) {
        m_Profiles = loaded.value;
    } else {
        m_Profiles = MakeDefaultPackageProfiles();
        m_Status = "Package profile load failed: " + loaded.error;
    }
    m_Loaded = true;
}

void PackagePanel::DrawProfile(const char* label, PackageProfile& profile, bool client) {
    if (ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Enabled", &profile.enabled);
        InputString("Preset", profile.preset);
        InputString("Build Mode", profile.buildMode);
        InputPath("Output Directory", profile.outputDir);
        ImGui::Checkbox("Include Debug Symbols", &profile.includeDebugSymbols);
        const char* button = client ? "Package Client" : "Package Server";
        if (ImGui::Button(button)) {
            PrepareCommand(label, profile, client);
        }
    }
}

void PackagePanel::PrepareCommand(const char* label, const PackageProfile& profile, bool client) {
    if (!profile.enabled) {
        m_Status = std::string(label) + " packaging is disabled";
        return;
    }
    const Result<std::filesystem::path> output = ResolvePackageOutputDir(Paths::Get().root, profile.outputDir);
    if (!output) {
        m_Status = output.error;
        return;
    }
    PackageProfile normalized = profile;
    normalized.outputDir = profile.outputDir.is_absolute() ? output.value : profile.outputDir;
    const PackageCommand command = client ? MakeClientPackageCommand(normalized) : MakeServerPackageCommand(normalized);
    m_Log = PackageCommandToString(command);
    m_Status = std::string(label) + " package command prepared";
    if (const Status saved = SavePackageProfiles(m_Profiles); !saved) {
        HK_EDITOR_WARN("Saving package profiles failed: {}", saved.error);
    }
}

void PackagePanel::OnImGui(EditorContext& /*context*/) {
    EnsureLoaded();
    if (!BeginWindow()) {
        EndWindow();
        return;
    }

    DrawProfile("Client", m_Profiles.client, true);
    DrawProfile("Server", m_Profiles.server, false);

    ImGui::Separator();
    if (!m_Status.empty()) {
        ImGui::TextUnformatted(m_Status.c_str());
    }
    if (ImGui::Button("Save Profiles")) {
        if (const Status saved = SavePackageProfiles(m_Profiles); saved) {
            m_Status = "Package profiles saved";
        } else {
            m_Status = "Package profile save failed: " + saved.error;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Log")) {
        m_Log.clear();
    }
    ImGui::BeginChild("Package Log", ImVec2(0.0f, 120.0f), true);
    if (!m_Log.empty()) {
        ImGui::TextUnformatted(m_Log.c_str());
    }
    ImGui::EndChild();

    EndWindow();
}

} // namespace Hockey

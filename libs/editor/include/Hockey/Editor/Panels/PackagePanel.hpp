#pragma once

#include "Hockey/Editor/Packaging/PackageProfile.hpp"
#include "Hockey/Editor/Panel.hpp"

#include <string>

namespace Hockey {

class PackagePanel final : public Panel {
public:
    PackagePanel();

    void OnImGui(EditorContext& context) override;

private:
    void EnsureLoaded();
    void DrawProfile(const char* label, PackageProfile& profile, bool client);
    void PrepareCommand(const char* label, const PackageProfile& profile, bool client);

    PackageProfiles m_Profiles;
    bool m_Loaded = false;
    std::string m_Status;
    std::string m_Log;
};

} // namespace Hockey

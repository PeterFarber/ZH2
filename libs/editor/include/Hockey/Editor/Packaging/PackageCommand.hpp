#pragma once

#include "Hockey/Editor/Packaging/PackageProfile.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Hockey {

struct PackageCommand {
    std::filesystem::path executable;
    std::vector<std::string> arguments;
    std::filesystem::path workingDirectory;
};

PackageCommand MakeClientPackageCommand(const PackageProfile& profile, const std::filesystem::path& projectRoot);
PackageCommand MakeServerPackageCommand(const PackageProfile& profile, const std::filesystem::path& projectRoot);
PackageCommand MakeClientPackageCommand(const PackageProfile& profile);
PackageCommand MakeServerPackageCommand(const PackageProfile& profile);
std::string PackageCommandToString(const PackageCommand& command);

} // namespace Hockey

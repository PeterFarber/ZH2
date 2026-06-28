#include "Hockey/Editor/Packaging/PackageCommand.hpp"

#include "Hockey/Core/Paths.hpp"

#include <sstream>

namespace Hockey {
namespace {

PackageCommand MakePackageCommand(const PackageProfile& profile, const std::filesystem::path& projectRoot,
                                  const char* windowsScript, const char* linuxScript) {
    PackageCommand command;
    command.workingDirectory = projectRoot;
#if HK_PLATFORM_WINDOWS
    (void)linuxScript;
    command.executable = projectRoot / "scripts" / "windows" / windowsScript;
    command.arguments = {"-Preset", profile.preset, "-OutputDir", profile.outputDir.generic_string(),
                         "-BuildMode", profile.buildMode};
    if (profile.includeDebugSymbols) {
        command.arguments.push_back("-IncludeDebugSymbols");
    }
#else
    (void)windowsScript;
    command.executable = projectRoot / "scripts" / "linux" / linuxScript;
    command.arguments = {"--preset", profile.preset, "--output-dir", profile.outputDir.generic_string(),
                         "--build-mode", profile.buildMode};
    if (profile.includeDebugSymbols) {
        command.arguments.push_back("--include-debug-symbols");
    }
#endif
    return command;
}

std::string QuoteIfNeeded(const std::string& text) {
    if (text.find_first_of(" \t\"") == std::string::npos) {
        return text;
    }
    std::string quoted = "\"";
    for (const char ch : text) {
        if (ch == '"') {
            quoted += "\\\"";
        } else {
            quoted += ch;
        }
    }
    quoted += '"';
    return quoted;
}

} // namespace

PackageCommand MakeClientPackageCommand(const PackageProfile& profile, const std::filesystem::path& projectRoot) {
    return MakePackageCommand(profile, projectRoot, "package_client.ps1", "package_client.sh");
}

PackageCommand MakeServerPackageCommand(const PackageProfile& profile, const std::filesystem::path& projectRoot) {
    return MakePackageCommand(profile, projectRoot, "package_server.ps1", "package_server.sh");
}

PackageCommand MakeClientPackageCommand(const PackageProfile& profile) {
    return MakeClientPackageCommand(profile, Paths::Get().root);
}

PackageCommand MakeServerPackageCommand(const PackageProfile& profile) {
    return MakeServerPackageCommand(profile, Paths::Get().root);
}

std::string PackageCommandToString(const PackageCommand& command) {
    std::ostringstream out;
    out << QuoteIfNeeded(command.executable.string());
    for (const std::string& argument : command.arguments) {
        out << ' ' << QuoteIfNeeded(argument);
    }
    return out.str();
}

} // namespace Hockey

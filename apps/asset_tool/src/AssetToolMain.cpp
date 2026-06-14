#include "AssetToolCommands.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

#include <string>
#include <vector>

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    const std::string root = commandLine.GetString("--root", ".");
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    Hockey::Log::Init(Hockey::Paths::LogFile("asset_tool.log"));

    // Collect positional (non-flag, non-flag-value) arguments. The only flag the
    // tool understands is --root <value>.
    std::vector<std::string> positionals;
    const std::vector<std::string>& args = commandLine.Args();
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg == "--root") {
            ++i; // skip its value
            continue;
        }
        if (arg.rfind("--", 0) == 0) {
            continue;
        }
        positionals.push_back(arg);
    }

    if (positionals.empty()) {
        Hockey::PrintAssetToolUsage();
        Hockey::Log::Shutdown();
        return 1;
    }

    const std::string command = positionals.front();
    const std::vector<std::string> commandArgs(positionals.begin() + 1, positionals.end());
    const int code = Hockey::RunAssetToolCommand(Hockey::Paths::Get().root, command, commandArgs);

    Hockey::Log::Shutdown();
    return code;
}

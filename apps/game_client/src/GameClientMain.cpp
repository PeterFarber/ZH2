#include "GameClientApp.hpp"
#include "Hockey/Core/CommandLine.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    if (commandLine.Has("--help") || commandLine.Has("-h")) {
        std::printf(
            "HockeyGameClient - playable hockey game client\n\n"
            "Usage: HockeyGameClient [options]\n\n"
            "Options:\n"
            "  --root <dir>      Project root directory (default: .)\n"
            "  --config <file>   Override client config TOML path\n"
            "  --log <file>      Override client log file path\n"
            "  --fps-limit <n>   Override the frame-rate cap\n"
            "  --max-frames <n>  Auto-quit after n rendered frames (testing)\n"
            "  --screenshot [p]  Save a screenshot from the first rendered frame\n"
            "  --screenshot-prefix <p>  Prefix for automated screenshot filenames\n"
            "  --vk-validation   Enable Vulkan validation layers\n"
            "  --physics-debug   Enable physics debug drawing\n"
            "  --help, -h        Print this message and exit\n");
        return 0;
    }
    GameClientApp app(commandLine);
    return app.Run();
}

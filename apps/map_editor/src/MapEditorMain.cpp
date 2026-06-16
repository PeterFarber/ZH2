#include "MapEditorApp.hpp"
#include "Hockey/Core/CommandLine.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    if (commandLine.Has("--help") || commandLine.Has("-h")) {
        std::printf(
            "HockeyMapEditor - Unity-style scene/map editor\n\n"
            "Usage: HockeyMapEditor [options]\n\n"
            "Options:\n"
            "  --root <dir>      Project root directory (default: .)\n"
            "  --config <file>   Override editor config TOML path\n"
            "  --log <file>      Override editor log file path\n"
            "  --scene <file>    Open the given scene on startup\n"
            "  --frames <n>      Auto-quit after N rendered frames (testing)\n"
            "  --vk-validation   Enable Vulkan validation layers\n"
            "  --help, -h        Print this message and exit\n");
        return 0;
    }
    MapEditorApp app(commandLine);
    return app.Run();
}

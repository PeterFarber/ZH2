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
            "  --root <dir>      Project root directory (default: inferred from executable)\n"
            "  --config <file>   Override editor config TOML path\n"
            "  --log <file>      Override editor log file path\n"
            "  --scene <file>    Open the given scene on startup\n"
            "  --max-frames <n>  Auto-quit after n rendered frames (testing)\n"
            "  --frames <n>      Alias for --max-frames\n"
            "  --capture-viewports  Save Scene and Game viewport screenshots\n"
            "  --capture-prefix <p> Prefix for automated viewport screenshots\n"
            "  --capture-width <n>  Automated viewport screenshot width (default: 1920)\n"
            "  --capture-height <n> Automated viewport screenshot height (default: 1080)\n"
            "  --screenshot      Save a screenshot of the full editor window\n"
            "  --screenshot-prefix <p> Prefix for automated screenshot filenames\n"
            "  --screenshot-frame <n> Frame to capture the full editor window (default: 3)\n"
            "  --vk-validation   Enable Vulkan validation layers\n"
            "  --help, -h        Print this message and exit\n");
        return 0;
    }
    MapEditorApp app(commandLine);
    return app.Run();
}

#include "DedicatedServerApp.hpp"
#include "Hockey/Core/CommandLine.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    if (commandLine.Has("--help") || commandLine.Has("-h")) {
        std::printf(
            "HockeyDedicatedServer - headless authoritative game server\n\n"
            "Usage: HockeyDedicatedServer [options]\n\n"
            "Options:\n"
            "  --root <dir>      Project root directory (default: .)\n"
            "  --config <file>   Override server config TOML path\n"
            "  --log <file>      Override server log file path\n"
            "  --tick-rate <hz>  Override the fixed simulation tick rate\n"
            "  --max-ticks <n>   Run a bounded simulation and exit after n ticks\n"
            "  --port <port>     Override the listen port\n"
            "  --help, -h        Print this message and exit\n");
        return 0;
    }
    DedicatedServerApp app(commandLine);
    return app.Run();
}

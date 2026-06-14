#include "GameClientApp.hpp"
#include "Hockey/Core/CommandLine.hpp"

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    GameClientApp app(commandLine);
    return app.Run();
}

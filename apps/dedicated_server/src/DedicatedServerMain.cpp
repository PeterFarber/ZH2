#include "DedicatedServerApp.hpp"
#include "Hockey/Core/CommandLine.hpp"

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    DedicatedServerApp app(commandLine);
    return app.Run();
}

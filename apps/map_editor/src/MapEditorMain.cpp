#include "MapEditorApp.hpp"
#include "Hockey/Core/CommandLine.hpp"

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    MapEditorApp app(commandLine);
    return app.Run();
}

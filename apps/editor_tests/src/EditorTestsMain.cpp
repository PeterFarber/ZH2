#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

// Each suite is implemented in its own translation unit and registered here.
void RunEditorVersionTests();
void RunEditorSettingsTests();
void RunSelectionTests();
void RunInspectorFieldTests();
void RunUndoRedoTests();
void RunCameraLightGizmoTests();
void RunSceneViewOverlayTests();
void RunToolTests();
void RunHockeyPhysicsSetupTests();
void RunEditorGameplayPreviewTests();
void RunSceneWorkflowTests();
void RunProjectBrowserTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("editor_tests.log"));

    RunEditorVersionTests();
    RunEditorSettingsTests();
    RunSelectionTests();
    RunInspectorFieldTests();
    RunUndoRedoTests();
    RunCameraLightGizmoTests();
    RunSceneViewOverlayTests();
    RunToolTests();
    RunHockeyPhysicsSetupTests();
    RunEditorGameplayPreviewTests();
    RunSceneWorkflowTests();
    RunProjectBrowserTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Editor tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Editor tests: %d passed, %d failed ====\n", stats.passed, stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}

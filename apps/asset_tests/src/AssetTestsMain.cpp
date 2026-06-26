#include "Test.hpp"

#include "Hockey/Core/CommandLine.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"

void RunAssetIDTests();
void RunAssetMetadataTests();
void RunAssetPathTests();
void RunAssetDatabaseTests();
void RunTextureImportTests();
void RunGltfImportTests();
void RunMaterialTests();
void RunShaderCookTests();
void RunDependencyTests();
void RunHotReloadTests();
void RunTextureMaxSizeTests();
void RunTangentGenerationTests();
void RunSceneAssetTests();
void RunBasicShapeAssetTests();

int main(int argc, char** argv) {
    Hockey::CommandLine commandLine(argc, argv);
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), commandLine.GetString("--root", "."));
    Hockey::Log::Init(Hockey::Paths::LogFile("asset_tests.log"));

    RunAssetIDTests();
    RunAssetMetadataTests();
    RunAssetPathTests();
    RunAssetDatabaseTests();
    RunTextureImportTests();
    RunGltfImportTests();
    RunMaterialTests();
    RunShaderCookTests();
    RunDependencyTests();
    RunHotReloadTests();
    RunTextureMaxSizeTests();
    RunTangentGenerationTests();
    RunSceneAssetTests();
    RunBasicShapeAssetTests();

    const HockeyTest::Stats& stats = HockeyTest::GetStats();
    HK_TEST_INFO("Asset tests completed: {} passed, {} failed", stats.passed, stats.failed);
    std::fprintf(stderr, "\n==== Asset tests: %d passed, %d failed ====\n", stats.passed,
                 stats.failed);

    Hockey::Log::Shutdown();
    return stats.failed == 0 ? 0 : 1;
}

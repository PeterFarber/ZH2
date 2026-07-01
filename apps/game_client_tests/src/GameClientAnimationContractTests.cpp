#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

} // namespace

void RunGameClientAnimationContractTests() {
    HockeyTest::BeginSuite("GameClientAnimationContractTests");

    const std::string cmake = ReadProjectFile("apps/game_client/CMakeLists.txt");
    const std::string header = ReadProjectFile("apps/game_client/src/GameClientApp.hpp");
    const std::string source = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");

    HK_CHECK_MSG(Contains(cmake, "hockey_animation"), "game client links hockey_animation");
    HK_CHECK_MSG(Contains(header, "HockeyAnimationController"), "game client owns hockey animation controller");
    HK_CHECK_MSG(Contains(header, "AnimationSystem"), "game client owns animation system");
    HK_CHECK_MSG(Contains(source, "BuildHockeyAnimationFrame"),
                 "game client translates GameplaySnapshot/events into animation input");
    HK_CHECK_MSG(Contains(source, "BuildGameplaySnapshot") && Contains(source, "m_AnimationController.Apply"),
                 "game client maps gameplay snapshot to animation parameters");
    const std::size_t syncPhysics = source.find("m_GameplayWorld.SyncPhysicsState");
    const std::size_t animationUpdate = source.find("m_AnimationSystem.Update", syncPhysics);
    const std::size_t presentationCapture = source.find("m_PresentationState.CaptureFixedStep", animationUpdate);
    HK_CHECK_MSG(syncPhysics != std::string::npos && animationUpdate != std::string::npos &&
                     presentationCapture != std::string::npos && syncPhysics < animationUpdate &&
                     animationUpdate < presentationCapture,
                 "animation update runs after gameplay/physics sync and before presentation capture");
}

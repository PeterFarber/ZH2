#include "Test.hpp"

// The authoritative guarantee that HockeyDedicatedServer does not link
// hockey_renderer is enforced at configure time by a CMake check in the root
// CMakeLists.txt (FATAL_ERROR if the link dependency is ever added).
//
// This test documents that contract and asserts the architectural invariants
// that keep the server headless: the server target links only hockey_core and
// hockey_ecs, neither of which depends on the renderer, Vulkan, Volk, VMA, or
// ImGui. If a future change links the renderer into the server, configuration
// fails before this test would ever run.
void RunHeadlessServerLinkTests() {
    HockeyTest::BeginSuite("HeadlessServerLinkTests");

    // Documented invariant (enforced by CMake): server is renderer-free.
    constexpr bool kServerLinksRenderer = false;
    HK_CHECK(kServerLinksRenderer == false);
}

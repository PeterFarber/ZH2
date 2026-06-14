#include "Test.hpp"

#include "Hockey/Editor/EditorApp.hpp"

#include <cstring>

void RunEditorVersionTests() {
    HockeyTest::BeginSuite("EditorVersionTests");

    const char* version = Hockey::EditorVersionString();
    HK_CHECK_MSG(version != nullptr, "editor version string is non-null");
    HK_CHECK_MSG(std::strlen(version) > 0, "editor version string is non-empty");
}

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

void RunEditorAudioCueContractTests() {
    HockeyTest::BeginSuite("EditorAudioCueContractTests");

    const std::string editorHeader = ReadProjectFile("libs/editor/include/Hockey/Editor/EditorAudioPreview.hpp");
    const std::string editorSource = ReadProjectFile("libs/editor/src/EditorAudioPreview.cpp");
    const std::string gameplayPreviewHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/EditorGameplayPreview.hpp");
    const std::string gameplayPreviewSource = ReadProjectFile("libs/editor/src/EditorGameplayPreview.cpp");
    const std::string physicsPreviewHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/EditorPhysicsPreview.hpp");
    const std::string physicsPreviewSource = ReadProjectFile("libs/editor/src/EditorPhysicsPreview.cpp");
    const std::string appSource = ReadProjectFile("libs/editor/src/EditorApp.cpp");
    const std::string settingsHeader =
        ReadProjectFile("libs/editor/include/Hockey/Editor/Panels/ProjectSettingsPanel.hpp");
    const std::string settingsSource = ReadProjectFile("libs/editor/src/Panels/ProjectSettingsPanel.cpp");
    const std::string clientSource = ReadProjectFile("apps/game_client/src/GameClientApp.cpp");
    const std::string defaults = ReadProjectFile("libs/core/src/RuntimeConfigDefaults.cpp");

    HK_CHECK_MSG(Contains(defaults, "[audio_cues]"), "code-owned defaults contain editable hockey audio cues");
    HK_CHECK_MSG(Contains(clientSource, "ResolveHockeyAudioCueMap"),
                 "game client loads cue assignments through the shared resolver");

    HK_CHECK_MSG(Contains(editorHeader, "HandleGameplayEvent"),
                 "editor audio preview can consume gameplay events");
    HK_CHECK_MSG(Contains(editorHeader, "HandlePhysicsContact"),
                 "editor audio preview can consume physics contacts");
    HK_CHECK_MSG(Contains(editorSource, "AudioCueId::Shot"), "editor routes shot events to shot cue");
    HK_CHECK_MSG(Contains(editorSource, "AudioCueId::PuckMetalCollision"),
                 "editor routes board/post contacts to metal collision cue");

    HK_CHECK_MSG(Contains(gameplayPreviewHeader, "DrainEvents"),
                 "gameplay preview exposes drained gameplay events for editor presentation");
    HK_CHECK_MSG(Contains(gameplayPreviewSource, "m_World.DrainEvents()"),
                 "gameplay preview drains events from GameplayWorld during fixed steps");
    HK_CHECK_MSG(Contains(physicsPreviewHeader, "DrainContactEvents"),
                 "physics preview exposes full contact events before clearing them");
    HK_CHECK_MSG(Contains(physicsPreviewSource, "m_PendingContactEvents"),
                 "physics preview preserves contact events for audio/debug consumers");

    HK_CHECK_MSG(Contains(appSource, "m_AudioPreview->HandleGameplayEvent"),
                 "editor playtest routes gameplay events to editor audio preview");
    HK_CHECK_MSG(Contains(appSource, "m_AudioPreview->HandlePhysicsContact"),
                 "editor playtest routes physics contacts to editor audio preview");

    HK_CHECK_MSG(Contains(settingsHeader, "EditorAudio"), "Project Settings has an editor audio page");
    HK_CHECK_MSG(Contains(settingsSource, "Audio Cues"), "Project Settings exposes audio cue assignments");
    HK_CHECK_MSG(Contains(settingsSource, "HockeyAudioCueConfigBindings"),
                 "Project Settings draws cue rows from the shared cue binding list");
    HK_CHECK_MSG(Contains(settingsSource, "AssetType::Audio"), "Project Settings filters cue pickers to audio assets");
}

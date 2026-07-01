#pragma once

#include <memory>
#include <optional>

#include <glm/vec3.hpp>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Core/Result.hpp"

namespace Hockey {

class AssetManager;
class AudioSystem;
class HockeyAudioController;
class Scene;
enum class AudioCueId;
struct AudioCueMap;
struct GameplayEvent;
struct PhysicsContactEvent;

class EditorAudioPreview {
public:
    EditorAudioPreview();
    ~EditorAudioPreview();

    EditorAudioPreview(const EditorAudioPreview&) = delete;
    EditorAudioPreview& operator=(const EditorAudioPreview&) = delete;

    Status Init(AssetManager* assetManager);
    void Shutdown();
    void Update(float deltaTime);
    void Preview(AssetID assetId);
    void Stop();
    void SetCueMap(AudioCueMap map);
    const AudioCueMap& CueMap() const;
    void Trigger(AudioCueId cue, std::optional<glm::vec3> position = std::nullopt, float volumeScale = 1.0f);
    void HandleGameplayEvent(const GameplayEvent& event);
    void HandlePhysicsContact(Scene& scene, const PhysicsContactEvent& contact);

    bool IsReady() const {
        return m_Ready;
    }

private:
    AssetManager* m_AssetManager = nullptr;
    std::unique_ptr<AudioSystem> m_AudioSystem;
    std::unique_ptr<HockeyAudioController> m_Controller;
    std::unique_ptr<AudioCueMap> m_CueMap;
    bool m_Ready = false;
};

} // namespace Hockey

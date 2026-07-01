#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include <glm/vec3.hpp>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Audio/AudioDevice.hpp"
#include "Hockey/Audio/AudioEvents.hpp"
#include "Hockey/Audio/AudioMixer.hpp"
#include "Hockey/ECS/System.hpp"

namespace Hockey {

class AssetManager;
struct AudioClip;

class AudioSystem final : public System, public IAudioCueSink {
public:
    explicit AudioSystem(AudioSettings settings = MakeDefaultAudioSettings());

    Status Initialize(AssetManager* assetManager);
    void Shutdown();
    bool IsInitialized() const;

    void OnStart(Scene& scene) override;
    void OnStop(Scene& scene) override;
    void OnUpdate(Scene& scene, float deltaTime) override;
    void Update(float deltaTime);

    void ApplySettings(const AudioSettings& settings);
    void PlayOneShot(AssetID clip, AudioBusType bus, float volume = 1.0f);
    void PlayOneShotAt(AssetID clip, glm::vec3 position, AudioBusType bus, float volume = 1.0f);
    void PlayCue(const AudioCue& cue) override;
    void StopAll();

    std::size_t DebugQueuedCommandCount() const;

private:
    std::shared_ptr<AudioClip> LoadClip(AssetID clip);

    AudioSettings m_Settings;
    AudioMixer m_Mixer;
    AudioDevice m_Device;
    AssetManager* m_AssetManager = nullptr;
    bool m_Initialized = false;
    std::unordered_map<std::uint64_t, std::shared_ptr<AudioClip>> m_ClipCache;
    std::size_t m_DebugQueuedCommandCount = 0;
};

} // namespace Hockey

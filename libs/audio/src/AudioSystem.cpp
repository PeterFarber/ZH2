#include "Hockey/Audio/AudioSystem.hpp"

#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/Assets/AudioAsset.hpp"
#include "Hockey/Audio/AudioClip.hpp"

namespace Hockey {

AudioSystem::AudioSystem(AudioSettings settings) : m_Settings(settings) {
    m_Mixer.ApplySettings(m_Settings);
}

Status AudioSystem::Initialize(AssetManager* assetManager) {
    if (m_Initialized) {
        return Status::Ok();
    }
    m_AssetManager = assetManager;
    m_Mixer.ApplySettings(m_Settings);

    if (!m_Settings.enabled) {
        m_Initialized = true;
        return Status::Ok();
    }
    if (m_AssetManager == nullptr && m_Settings.backend != AudioBackendMode::Null) {
        return Status::Fail("audio asset manager is required for non-null playback");
    }

    const Status status = m_Device.Init(m_Settings);
    if (!status) {
        m_AssetManager = nullptr;
        return status;
    }
    m_Initialized = true;
    return Status::Ok();
}

void AudioSystem::Shutdown() {
    StopAll();
    m_ClipCache.clear();
    m_Device.Shutdown();
    m_AssetManager = nullptr;
    m_Initialized = false;
}

bool AudioSystem::IsInitialized() const {
    return m_Initialized;
}

void AudioSystem::OnStart(Scene& scene) {
    (void)scene;
}

void AudioSystem::OnStop(Scene& scene) {
    (void)scene;
    StopAll();
}

void AudioSystem::OnUpdate(Scene& scene, float deltaTime) {
    (void)scene;
    Update(deltaTime);
}

void AudioSystem::Update(float deltaTime) {
    (void)deltaTime;
    if (m_Initialized && m_Settings.enabled) {
        m_Device.Update();
    }
}

void AudioSystem::ApplySettings(const AudioSettings& settings) {
    m_Settings = settings;
    m_Mixer.ApplySettings(m_Settings);
}

void AudioSystem::PlayOneShot(AssetID clip, AudioBusType bus, float volume) {
    if (!m_Initialized || !clip.IsValid()) {
        return;
    }
    ++m_DebugQueuedCommandCount;
    if (!m_Settings.enabled || m_Device.IsNullBackend()) {
        return;
    }
    std::shared_ptr<AudioClip> decoded = LoadClip(clip);
    if (!decoded) {
        return;
    }
    (void)m_Device.PlayOneShot(std::move(decoded), bus, volume, m_Mixer);
}

void AudioSystem::PlayOneShotAt(AssetID clip, glm::vec3 position, AudioBusType bus, float volume) {
    (void)position;
    PlayOneShot(clip, bus, volume);
}

void AudioSystem::PlayCue(const AudioCue& cue) {
    if (cue.position.has_value()) {
        PlayOneShotAt(cue.clip, *cue.position, cue.bus, cue.volume);
    } else {
        PlayOneShot(cue.clip, cue.bus, cue.volume);
    }
}

void AudioSystem::StopAll() {
    m_Device.StopAll();
}

std::size_t AudioSystem::DebugQueuedCommandCount() const {
    return m_DebugQueuedCommandCount;
}

std::shared_ptr<AudioClip> AudioSystem::LoadClip(AssetID clip) {
    const auto cached = m_ClipCache.find(clip.Value());
    if (cached != m_ClipCache.end()) {
        return cached->second;
    }
    if (m_AssetManager == nullptr) {
        return nullptr;
    }
    Result<std::shared_ptr<AudioAsset>> asset = m_AssetManager->Load<AudioAsset>(clip);
    if (!asset || !asset.value) {
        return nullptr;
    }
    Result<AudioClip> decoded = AudioClip::Decode(*asset.value);
    if (!decoded) {
        return nullptr;
    }
    auto stored = std::make_shared<AudioClip>(std::move(decoded.value));
    m_ClipCache[clip.Value()] = stored;
    return stored;
}

} // namespace Hockey

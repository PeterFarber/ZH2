#include "Hockey/Editor/EditorAudioPreview.hpp"

#include <algorithm>
#include <string>

#include "Hockey/Audio/AudioBus.hpp"
#include "Hockey/Audio/AudioEvents.hpp"
#include "Hockey/Audio/AudioSettings.hpp"
#include "Hockey/Audio/AudioSystem.hpp"
#include "Hockey/Audio/HockeyAudioController.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Gameplay/GameplayEvents.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsEvents.hpp"

namespace Hockey {
namespace {

bool UsesMetalCollisionCue(Scene& scene, UUID id) {
    Entity entity = scene.FindEntityByUUID(id);
    if (!entity) {
        return false;
    }

    std::string material;
    if (entity.HasComponent<PhysicsMaterialComponent>()) {
        material = entity.GetComponent<PhysicsMaterialComponent>().materialName;
    } else if (entity.HasComponent<RigidBodyComponent>()) {
        material = entity.GetComponent<RigidBodyComponent>().materialName;
    }

    return material.find("Metal") != std::string::npos || material.find("Board") != std::string::npos ||
           material.find("Glass") != std::string::npos || material.find("Goal") != std::string::npos;
}

} // namespace

EditorAudioPreview::EditorAudioPreview()
    : m_AudioSystem(std::make_unique<AudioSystem>()),
      m_Controller(std::make_unique<HockeyAudioController>()),
      m_CueMap(std::make_unique<AudioCueMap>()) {}

EditorAudioPreview::~EditorAudioPreview() {
    Shutdown();
}

Status EditorAudioPreview::Init(AssetManager* assetManager) {
    Shutdown();

    m_AssetManager = assetManager;
    AudioSettings settings = MakeDefaultAudioSettings();
    settings.enabled = assetManager != nullptr;
    settings.backend = AudioBackendMode::Auto;
    m_AudioSystem = std::make_unique<AudioSystem>(settings);

    const Status status = m_AudioSystem->Initialize(assetManager);
    if (!status) {
        m_AssetManager = nullptr;
        m_Ready = false;
        return status;
    }

    m_Controller->SetSink(m_AudioSystem.get());
    m_Controller->SetCueMap(*m_CueMap);
    m_Ready = true;
    return Status::Ok();
}

void EditorAudioPreview::Shutdown() {
    m_Controller->SetSink(nullptr);
    if (m_AudioSystem != nullptr && m_AudioSystem->IsInitialized()) {
        m_AudioSystem->Shutdown();
    }
    m_AssetManager = nullptr;
    m_Ready = false;
}

void EditorAudioPreview::Update(float deltaTime) {
    if (m_Ready && m_AudioSystem != nullptr) {
        m_AudioSystem->Update(deltaTime);
    }
}

void EditorAudioPreview::Preview(AssetID assetId) {
    if (m_Ready && m_AudioSystem != nullptr) {
        m_AudioSystem->PlayOneShot(assetId, AudioBusType::SFX, 1.0f);
    }
}

void EditorAudioPreview::Stop() {
    if (m_AudioSystem != nullptr) {
        m_AudioSystem->StopAll();
    }
}

void EditorAudioPreview::SetCueMap(AudioCueMap map) {
    *m_CueMap = map;
    m_Controller->SetCueMap(*m_CueMap);
}

const AudioCueMap& EditorAudioPreview::CueMap() const {
    return *m_CueMap;
}

void EditorAudioPreview::Trigger(AudioCueId cue, std::optional<glm::vec3> position, float volumeScale) {
    if (m_Ready) {
        m_Controller->Trigger(cue, position, volumeScale);
    }
}

void EditorAudioPreview::HandleGameplayEvent(const GameplayEvent& event) {
    switch (event.type) {
    case GameplayEventType::CountdownBeep:
    case GameplayEventType::FaceoffStarted:
        Trigger(AudioCueId::Countdown, event.position);
        break;
    case GameplayEventType::PuckShot:
        Trigger(AudioCueId::Shot, event.position);
        break;
    case GameplayEventType::GoalieShieldStarted:
        Trigger(AudioCueId::GoalieShield, event.position);
        break;
    case GameplayEventType::PlayerBoosted:
        Trigger(AudioCueId::PlayerBoost, event.position, 0.8f);
        break;
    default:
        break;
    }
}

void EditorAudioPreview::HandlePhysicsContact(Scene& scene, const PhysicsContactEvent& contact) {
    if (contact.type != PhysicsContactEvent::Type::Started) {
        return;
    }

    const AudioCueId cue = (UsesMetalCollisionCue(scene, contact.entityA) || UsesMetalCollisionCue(scene, contact.entityB))
                               ? AudioCueId::PuckMetalCollision
                               : AudioCueId::PuckCollision;
    const float volume = contact.impulse > 0.0f ? std::clamp(contact.impulse / 8.0f, 0.2f, 1.0f) : 0.35f;
    Trigger(cue, contact.contactPoint, volume);
}

} // namespace Hockey

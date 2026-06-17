#include "Hockey/Editor/EditorGameplayPreview.hpp"

#include <filesystem>

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Input.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/Editor/EditorPhysicsPreview.hpp"

#include <glm/geometric.hpp>
#include <imgui.h>

namespace Hockey {

void EditorGameplayPreview::Configure(const GameplaySettings& settings) {
    m_Settings = settings;
    if (m_Settings.fixedDeltaSeconds > 0.0f) {
        m_Timestep.SetTickRate(1.0 / static_cast<double>(m_Settings.fixedDeltaSeconds));
    }
}

Status EditorGameplayPreview::Start(Scene& scene, EditorPhysicsPreview& physicsPreview) {
    if (m_Active) {
        return Status::Ok();
    }

    if (Status saved = SaveAuthoringSnapshot(scene); !saved) {
        return saved;
    }

    m_StartedPhysicsPreview = !physicsPreview.IsActive();
    if (m_StartedPhysicsPreview) {
        physicsPreview.Start(scene);
    }
    if (!physicsPreview.IsActive()) {
        ClearSnapshot();
        m_StartedPhysicsPreview = false;
        return Status::Fail("Gameplay preview requires an active physics preview");
    }

    if (Status initialized = m_World.Init(scene, &physicsPreview.World().World(), m_Settings); !initialized) {
        m_World.Shutdown();
        if (m_StartedPhysicsPreview) {
            physicsPreview.Stop(scene);
        }
        RestoreAuthoringSnapshot(scene);
        ClearSnapshot();
        m_StartedPhysicsPreview = false;
        return initialized;
    }

    m_Timestep.Reset();
    m_LocalInputSequence = 0;
    m_Tick = 0;
    m_Active = true;
    m_Running = false;
    return Status::Ok();
}

void EditorGameplayPreview::Stop(Scene& scene, EditorPhysicsPreview& physicsPreview) {
    if (!m_Active) {
        return;
    }

    m_World.Shutdown();
    m_Active = false;
    m_Running = false;
    RestoreAuthoringSnapshot(scene);

    if (m_StartedPhysicsPreview) {
        physicsPreview.Stop(scene);
    } else if (physicsPreview.IsActive()) {
        physicsPreview.Reset(scene);
    }
    m_StartedPhysicsPreview = false;
    ClearSnapshot();
}

void EditorGameplayPreview::Play() {
    if (m_Active) {
        m_Running = true;
    }
}

void EditorGameplayPreview::Pause() {
    m_Running = false;
}

void EditorGameplayPreview::StepOnce(Scene& scene, EditorPhysicsPreview& physicsPreview) {
    if (!m_Active) {
        return;
    }
    m_Running = false;
    StepFixed(scene, physicsPreview, m_Settings.fixedDeltaSeconds);
}

void EditorGameplayPreview::Reset(Scene& scene, EditorPhysicsPreview& physicsPreview) {
    if (!m_Active) {
        return;
    }
    m_World.Shutdown();
    m_Running = false;
    RestoreAuthoringSnapshot(scene);
    if (physicsPreview.IsActive()) {
        physicsPreview.Reset(scene);
    }
    m_Timestep.Reset();
    m_LocalInputSequence = 0;
    m_Tick = 0;
    if (Status initialized = m_World.Init(scene, &physicsPreview.World().World(), m_Settings); !initialized) {
        m_Active = false;
        ClearSnapshot();
    }
}

void EditorGameplayPreview::Update(Scene& scene, EditorPhysicsPreview& physicsPreview, float deltaTime) {
    if (!m_Active || !m_Running) {
        return;
    }

    const int steps = m_Timestep.Accumulate(static_cast<double>(deltaTime));
    const float fixedDelta = static_cast<float>(m_Timestep.GetFixedDeltaSeconds());
    for (int i = 0; i < steps; ++i) {
        StepFixed(scene, physicsPreview, fixedDelta);
        m_Timestep.AdvanceTick();
    }
}

Status EditorGameplayPreview::SaveAuthoringSnapshot(Scene& scene) {
    const std::filesystem::path dir = Paths::DataFile("temp/editor_gameplay_preview");
    if (Status created = FileSystem::CreateDirectories(dir); !created) {
        return created;
    }
    m_SnapshotPath = dir / "authoring_snapshot.scene.yaml";
    return SceneSerializer(scene).Serialize(m_SnapshotPath);
}

Status EditorGameplayPreview::RestoreAuthoringSnapshot(Scene& scene) {
    if (m_SnapshotPath.empty()) {
        return Status::Ok();
    }
    return SceneSerializer(scene).Deserialize(m_SnapshotPath);
}

void EditorGameplayPreview::ClearSnapshot() {
    if (!m_SnapshotPath.empty()) {
        std::error_code ec;
        std::filesystem::remove(m_SnapshotPath, ec);
        m_SnapshotPath.clear();
    }
}

GameplayInputFrame EditorGameplayPreview::BuildLocalInput(std::uint64_t simulationTick) {
    GameplayInputFrame input;
    input.playerIndex = 0;
    input.inputSequence = ++m_LocalInputSequence;
    input.simulationTick = simulationTick;

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantTextInput) {
        return input;
    }

    input.move.x = (Input::IsKeyDown(KeyCode::D) ? 1.0f : 0.0f) - (Input::IsKeyDown(KeyCode::A) ? 1.0f : 0.0f);
    input.move.y = (Input::IsKeyDown(KeyCode::W) ? 1.0f : 0.0f) - (Input::IsKeyDown(KeyCode::S) ? 1.0f : 0.0f);
    if (glm::dot(input.move, input.move) > 1.0f) {
        input.move = glm::normalize(input.move);
    }

    input.aim.x = (Input::IsKeyDown(KeyCode::Right) ? 1.0f : 0.0f) -
                  (Input::IsKeyDown(KeyCode::Left) ? 1.0f : 0.0f);
    input.aim.y = (Input::IsKeyDown(KeyCode::Up) ? 1.0f : 0.0f) -
                  (Input::IsKeyDown(KeyCode::Down) ? 1.0f : 0.0f);
    if (glm::dot(input.aim, input.aim) > 1.0f) {
        input.aim = glm::normalize(input.aim);
    }

    input.sprint = Input::IsMouseButtonDown(MouseButton::Right);
    input.shootPressed = Input::WasKeyPressed(KeyCode::Space);
    input.shootHeld = Input::IsKeyDown(KeyCode::Space);
    input.shootReleased = Input::WasKeyReleased(KeyCode::Space);
    input.passPressed = Input::WasMouseButtonPressed(MouseButton::Left);
    input.passHeld = Input::IsMouseButtonDown(MouseButton::Left);
    input.passReleased = Input::WasMouseButtonReleased(MouseButton::Left);
    input.checkPressed = Input::WasMouseButtonPressed(MouseButton::Right);
    input.pokeCheckPressed = Input::WasMouseButtonPressed(MouseButton::Middle);

    return input;
}

void EditorGameplayPreview::StepFixed(Scene& scene, EditorPhysicsPreview& physicsPreview, float fixedDeltaSeconds) {
    if (!m_World.IsInitialized() || !physicsPreview.IsActive()) {
        return;
    }
    m_World.PushInput(BuildLocalInput(m_Tick));
    m_World.FixedUpdate(scene, fixedDeltaSeconds, m_Tick);
    physicsPreview.World().OnFixedUpdate(scene, fixedDeltaSeconds);
    ++m_Tick;
}

} // namespace Hockey

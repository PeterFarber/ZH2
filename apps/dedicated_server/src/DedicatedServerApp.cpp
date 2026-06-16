#include "DedicatedServerApp.hpp"
#include "Hockey/Core/CrashHandler.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/JobSystem.hpp"
#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/Platform.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Physics/Physics.hpp"
#include "Hockey/Physics/PhysicsComponents.hpp"
#include "Hockey/Physics/PhysicsMaterial.hpp"
#include "Hockey/Physics/PhysicsSettings.hpp"
#include "Hockey/Physics/PhysicsSystem.hpp"
#include "Hockey/Physics/PhysicsWorld.hpp"
#include <algorithm>
#include <filesystem>
#include <memory>

bool DedicatedServerApp::OnInit() {
    const auto& cmd = GetCommandLine();
    auto root = cmd.GetString("--root", ".");
    Hockey::Paths::Init(Hockey::Platform::ExecutablePath(), root);
    const auto logPath = cmd.Has("--log") ? Hockey::Paths::Resolve(cmd.GetString("--log"))
                                          : Hockey::Paths::LogFile("server.log");
    Hockey::Log::Init(logPath);
    Hockey::CrashHandler::Install();
    Hockey::JobSystem::Init();
    const auto configPath = cmd.Has("--config") ? Hockey::Paths::Resolve(cmd.GetString("--config"))
                                                : Hockey::Paths::ConfigFile("server.toml");
    m_Config.Load(configPath);
    SetSleepWhenIdle(m_Config.GetBool("app.sleep_when_idle", true));

    double tickRate = m_Config.GetDouble("server.tick_rate", 60.0);
    if (GetCommandLine().Has("--tick-rate")) {
        tickRate = GetCommandLine().GetDouble("--tick-rate", tickRate);
    }
    if (tickRate <= 0.0) {
        tickRate = 60.0;
    }
    SetTickRate(tickRate);
    m_TicksPerLog = static_cast<uint64_t>(std::max(1.0, tickRate));

    HK_SERVER_INFO("Dedicated server started on {} at {} Hz (port {})", Hockey::Platform::OSName(), tickRate,
                   m_Config.GetInt("server.port", 27020));

    // Register physics component serialization/validation + materials before
    // loading any scene so physics components deserialize and validate.
    Hockey::RegisterPhysicsComponents();
    Hockey::PhysicsMaterialRegistry::Get().RegisterBuiltIns();

    m_Scene.SetMode(Hockey::SceneMode::Server);
    const std::string startupScene = m_Config.GetString("scene.startup_scene", "");
    if (!startupScene.empty()) {
        const std::filesystem::path scenePath = Hockey::Paths::Get().root / startupScene;
        if (Hockey::FileSystem::Exists(scenePath)) {
            Hockey::SceneSerializer serializer(m_Scene);
            if (!serializer.Deserialize(scenePath)) {
                HK_SERVER_INFO("Could not load startup scene; running empty Server scene");
            }
        } else {
            HK_SERVER_INFO("Startup scene '{}' not found; running empty Server scene", scenePath.string());
        }
    }

    if (m_Config.GetBool("scene.validate_on_load", true)) {
        const auto issues = Hockey::SceneValidator::Validate(m_Scene);
        int errors = 0;
        int warnings = 0;
        for (const auto& issue : issues) {
            if (issue.severity == Hockey::SceneValidationIssue::Severity::Error) {
                ++errors;
                HK_SERVER_INFO("Scene validation error: {}", issue.message);
            } else {
                ++warnings;
                HK_SERVER_INFO("Scene validation warning: {}", issue.message);
            }
        }
        HK_SERVER_INFO("Scene '{}' loaded with {} entities ({} errors, {} warnings)", m_Scene.GetName(),
                       m_Scene.EntityCount(), errors, warnings);
    } else {
        HK_SERVER_INFO("Scene '{}' loaded with {} entities", m_Scene.GetName(), m_Scene.EntityCount());
    }

    if (m_Config.GetBool("physics.enabled", true)) {
        if (!Hockey::Physics::Init()) {
            HK_SERVER_INFO("Physics failed to initialise; running without physics");
        } else {
            Hockey::PhysicsSettings physicsSettings;
            Hockey::LoadPhysicsSettings(m_Config, physicsSettings);
            m_PhysicsDebug = physicsSettings.enableDebugDraw;

            auto physicsSystem = std::make_unique<Hockey::PhysicsSystem>(physicsSettings);
            m_PhysicsSystem = physicsSystem.get();
            m_Scene.AddSystem(std::move(physicsSystem));

            // Server scenes are authoritative: start the simulation immediately.
            m_Scene.OnSimulationStart();
            HK_SERVER_INFO("Physics enabled ({} bodies, gravity {:.2f})", m_PhysicsSystem->World().BodyCount(),
                           physicsSettings.gravity.y);
        }
    } else {
        HK_SERVER_INFO("Physics disabled by config");
    }
    return true;
}

void DedicatedServerApp::OnShutdown() {
    HK_SERVER_INFO("Server shutdown after {} ticks", m_Tick);
    if (m_PhysicsSystem != nullptr) {
        m_Scene.OnSimulationStop();
        m_Scene.ClearSystems(); // destroy physics world before global Jolt teardown
        m_PhysicsSystem = nullptr;
        Hockey::Physics::Shutdown();
    }
    Hockey::JobSystem::Shutdown();
    Hockey::CrashHandler::Shutdown();
    Hockey::Log::Shutdown();
}

void DedicatedServerApp::OnFixedUpdate(double fixedDeltaSeconds) {
    m_Scene.OnFixedUpdate(static_cast<float>(fixedDeltaSeconds));
    ++m_Tick;

    if (m_PhysicsSystem != nullptr) {
        const std::size_t contacts = m_PhysicsSystem->World().DrainContactEvents().size();
        const std::size_t triggers = m_PhysicsSystem->World().DrainTriggerEvents().size();
        if (m_PhysicsDebug && (contacts > 0 || triggers > 0)) {
            HK_SERVER_INFO("Physics tick {}: {} contact event(s), {} trigger event(s)", m_Tick, contacts, triggers);
        }
    }

    if (m_Tick % m_TicksPerLog == 0) {
        HK_SERVER_INFO("Server tick {}", m_Tick);
    }
}

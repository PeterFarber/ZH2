#include "Hockey/Editor/SceneWorkflow.hpp"

#include <string>

#include "Hockey/Core/Log.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/SceneSerializer.hpp"
#include "Hockey/ECS/SceneValidator.hpp"
#include "Hockey/Editor/EditorContext.hpp"

namespace Hockey {

namespace {

constexpr const char* kSceneSuffix = ".scene.yaml";

std::string SanitizeFileStem(std::string name) {
    for (char& chr : name) {
        if (chr == '/' || chr == '\\' || chr == ':' || chr == '*' || chr == '?' || chr == '"' || chr == '<' ||
            chr == '>' || chr == '|' || chr == ' ') {
            chr = '_';
        }
    }
    return name.empty() ? std::string("Untitled") : name;
}

void LogValidation(const Scene& scene) {
    const auto issues = SceneValidator::Validate(scene);
    for (const SceneValidationIssue& issue : issues) {
        if (issue.severity == SceneValidationIssue::Severity::Error) {
            HK_EDITOR_ERROR("[validation] {}", issue.message);
        } else {
            HK_EDITOR_WARN("[validation] {}", issue.message);
        }
    }
}

} // namespace

std::filesystem::path SceneWorkflow::AutosaveDirectory() {
    return Paths::Get().temp / "autosaves";
}

std::filesystem::path SceneWorkflow::AutosavePathFor(const Scene& scene) {
    return AutosaveDirectory() / (SanitizeFileStem(scene.GetName()) + ".autosave.scene.yaml");
}

std::filesystem::path SceneWorkflow::EnsureSceneExtension(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    if (name.size() >= std::char_traits<char>::length(kSceneSuffix)) {
        const std::string suffix = name.substr(name.size() - std::char_traits<char>::length(kSceneSuffix));
        if (suffix == kSceneSuffix) {
            return path;
        }
    }
    // Strip any existing extension(s) before appending the canonical suffix.
    std::filesystem::path result = path;
    result.replace_extension();
    return result.parent_path() / (result.filename().string() + kSceneSuffix);
}

void SceneWorkflow::NewScene(EditorContext& context, bool createDefaults) {
    if (context.activeScene == nullptr) {
        return;
    }
    Scene& scene = *context.activeScene;
    scene.Clear();
    scene.SetName("Untitled Scene");
    scene.SetMode(SceneMode::Edit);

    if (createDefaults) {
        Entity light = scene.CreateEntity("Directional Light");
        LightComponent& component = light.AddComponent<LightComponent>();
        component.type = LightComponent::Type::Directional;
        light.GetComponent<TransformComponent>().localRotation = {50.0f, -30.0f, 0.0f};
    }

    context.selection.Clear();
    context.undoRedo.Clear();
    context.activeScenePath.clear();
    context.MarkDirty(false);
    ResetAutosaveTimer();
    HK_EDITOR_INFO("New scene created");
}

Status SceneWorkflow::OpenScene(EditorContext& context, const std::filesystem::path& path) {
    if (context.activeScene == nullptr) {
        return Status::Fail("No active scene");
    }
    SceneSerializer serializer(*context.activeScene);
    if (const Status status = serializer.Deserialize(path); !status) {
        return status;
    }

    context.activeScene->SetMode(SceneMode::Edit);
    context.activeScenePath = path;
    context.selection.Clear();
    context.undoRedo.Clear();
    context.MarkDirty(false);
    context.settings.AddRecentScene(path);
    ResetAutosaveTimer();

    if (context.settings.validateAfterLoad) {
        LogValidation(*context.activeScene);
    }
    return Status::Ok();
}

Status SceneWorkflow::SaveScene(EditorContext& context) {
    if (context.activeScene == nullptr) {
        return Status::Fail("No active scene");
    }
    if (context.activeScenePath.empty()) {
        return Status::Fail("Scene has no path; use Save As");
    }
    if (context.settings.validateBeforeSave) {
        LogValidation(*context.activeScene); // warnings/errors logged, do not block
    }
    SceneSerializer serializer(*context.activeScene);
    if (const Status status = serializer.Serialize(context.activeScenePath); !status) {
        return status;
    }
    context.MarkDirty(false);
    ResetAutosaveTimer();
    return Status::Ok();
}

Status SceneWorkflow::SaveSceneAs(EditorContext& context, const std::filesystem::path& path) {
    if (context.activeScene == nullptr) {
        return Status::Fail("No active scene");
    }
    const std::filesystem::path target = EnsureSceneExtension(path);
    if (context.settings.validateBeforeSave) {
        LogValidation(*context.activeScene);
    }
    SceneSerializer serializer(*context.activeScene);
    if (const Status status = serializer.Serialize(target); !status) {
        return status;
    }
    context.activeScenePath = target;
    context.MarkDirty(false);
    context.settings.AddRecentScene(target);
    ResetAutosaveTimer();
    return Status::Ok();
}

void SceneWorkflow::TickAutosave(EditorContext& context, float deltaTime) {
    if (!context.settings.autosaveEnabled || context.activeScene == nullptr) {
        m_AutosaveTimer = 0.0f;
        return;
    }
    m_AutosaveTimer += deltaTime;
    if (m_AutosaveTimer < static_cast<float>(context.settings.autosaveIntervalSeconds)) {
        return;
    }
    m_AutosaveTimer = 0.0f;
    if (!context.sceneDirty) {
        return;
    }
    const std::filesystem::path path = AutosavePathFor(*context.activeScene);
    SceneSerializer serializer(*context.activeScene);
    if (const Status status = serializer.Serialize(path); status) {
        HK_EDITOR_INFO("Autosaved to {}", path.string());
    } else {
        HK_EDITOR_WARN("Autosave failed: {}", status.error);
    }
}

} // namespace Hockey

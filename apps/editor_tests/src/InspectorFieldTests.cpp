#include "Test.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/Editor/Inspector/ComponentInspector.hpp"
#include "Hockey/Editor/Inspector/FieldDrawers.hpp"

using namespace Hockey;

namespace {

// Locates a registered field by component + field name. Returns nullptr if not
// found so the test fails loudly rather than dereferencing garbage.
const FieldMetadata* FindField(const ComponentMetadata& md, const char* fieldName) {
    for (const FieldMetadata& field : md.fields) {
        if (field.name == fieldName) {
            return &field;
        }
    }
    return nullptr;
}

// Synthetic component used to exercise vec2/vec4 field access, which no Phase 2
// component currently registers.
struct VecProbe {
    glm::vec2 two{0.0f};
    glm::vec4 four{0.0f};
};

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(std::string_view{needle}) != std::string::npos;
}

} // namespace

void RunInspectorFieldTests() {
    HockeyTest::BeginSuite("InspectorFieldTests");

    ComponentRegistry& registry = ComponentRegistry::Get();
    registry.RegisterPhase2Components();

    Scene scene("InspectorFieldScene");
    Entity entity = scene.CreateEntity("Probe");

    // --- bool field edit (MeshRenderer.visible) -----------------------------
    {
        entity.AddComponent<MeshRendererComponent>();
        const ComponentMetadata* md = registry.FindByName("MeshRendererComponent");
        HK_CHECK_MSG(md != nullptr, "MeshRenderer metadata registered");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "Visible") : nullptr;
        HK_CHECK_MSG(field != nullptr, "Visible field present");
        if (md != nullptr && field != nullptr) {
            void* data = md->getData(entity);
            HK_CHECK_MSG(data != nullptr, "getData returns component pointer");
            bool* value = static_cast<bool*>(FieldDrawers::FieldPointer(data, *field));
            *value = false;
            HK_CHECK_MSG(!entity.GetComponent<MeshRendererComponent>().visible, "bool field written via offset");
        }
    }

    // --- bool field edit (SpawnPoint.faceoffSpawn) --------------------------
    {
        entity.AddComponent<SpawnPointComponent>();
        const ComponentMetadata* md = registry.FindByName("SpawnPointComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "FaceoffSpawn") : nullptr;
        HK_CHECK_MSG(field != nullptr, "FaceoffSpawn field present");
        if (md != nullptr && field != nullptr) {
            bool* value = static_cast<bool*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = true;
            HK_CHECK(entity.GetComponent<SpawnPointComponent>().faceoffSpawn);
        }
    }

    // --- spawn goalie checkbox helper ---------------------------------------
    {
        Entity spawn = scene.CreateEntity("Role Toggle Spawn");
        spawn.AddComponent<SpawnPointComponent>();

        HK_CHECK(!SpawnRoleInspector::IsGoalieSpawn(spawn));
        SpawnRoleInspector::SetGoalieSpawn(spawn, true);
        HK_CHECK(SpawnRoleInspector::IsGoalieSpawn(spawn));
        HK_CHECK(spawn.HasComponent<PlayerRoleComponent>());
        HK_CHECK(spawn.GetComponent<PlayerRoleComponent>().role == PlayerRole::Goalie);

        SpawnRoleInspector::SetGoalieSpawn(spawn, false);
        HK_CHECK(!SpawnRoleInspector::IsGoalieSpawn(spawn));
        HK_CHECK(spawn.HasComponent<PlayerRoleComponent>());
        HK_CHECK(spawn.GetComponent<PlayerRoleComponent>().role == PlayerRole::Skater);
    }

    // --- float field edit (Camera.fovDegrees) -------------------------------
    {
        entity.AddComponent<CameraComponent>();
        const ComponentMetadata* md = registry.FindByName("CameraComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "FovDegrees") : nullptr;
        HK_CHECK_MSG(field != nullptr, "FovDegrees field present");
        if (md != nullptr && field != nullptr) {
            float* value = static_cast<float*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = 75.0f;
            HK_CHECK_NEAR(entity.GetComponent<CameraComponent>().fovDegrees, 75.0f, 1e-5);
        }

        const FieldMetadata* followPlayerField = md != nullptr ? FindField(*md, "FollowPlayer") : nullptr;
        HK_CHECK_MSG(followPlayerField != nullptr, "FollowPlayer field present");
        if (md != nullptr && followPlayerField != nullptr) {
            bool* value = static_cast<bool*>(FieldDrawers::FieldPointer(md->getData(entity), *followPlayerField));
            *value = true;
            HK_CHECK(entity.GetComponent<CameraComponent>().followPlayer);
        }

        const FieldMetadata* followOffsetField = md != nullptr ? FindField(*md, "FollowOffset") : nullptr;
        const FieldMetadata* followRotationField = md != nullptr ? FindField(*md, "FollowRotation") : nullptr;
        HK_CHECK_MSG(followOffsetField != nullptr, "FollowOffset field present");
        HK_CHECK_MSG(followRotationField != nullptr, "FollowRotation field present");
        if (md != nullptr && followOffsetField != nullptr && followRotationField != nullptr) {
            auto& camera = entity.GetComponent<CameraComponent>();
            camera.followPlayer = false;
            HK_CHECK(!FieldDrawers::IsFieldVisible(*md, md->getData(entity), *followOffsetField));
            HK_CHECK(!FieldDrawers::IsFieldVisible(*md, md->getData(entity), *followRotationField));

            camera.followPlayer = true;
            HK_CHECK(FieldDrawers::IsFieldVisible(*md, md->getData(entity), *followOffsetField));
            HK_CHECK(FieldDrawers::IsFieldVisible(*md, md->getData(entity), *followRotationField));

            auto* offset = static_cast<glm::vec3*>(FieldDrawers::FieldPointer(md->getData(entity), *followOffsetField));
            *offset = glm::vec3(3.0f, 4.0f, 5.0f);
            auto* rotation =
                static_cast<glm::vec3*>(FieldDrawers::FieldPointer(md->getData(entity), *followRotationField));
            *rotation = glm::vec3(-25.0f, 10.0f, 0.0f);
            HK_CHECK_NEAR(camera.followOffset.z, 5.0f, 1e-5);
            HK_CHECK_NEAR(camera.followRotation.x, -25.0f, 1e-5);
        }
    }

    // --- string field edit (Rink.rinkName) ----------------------------------
    {
        entity.AddComponent<RinkComponent>();
        const ComponentMetadata* md = registry.FindByName("RinkComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "RinkName") : nullptr;
        HK_CHECK_MSG(field != nullptr, "RinkName field present");
        if (md != nullptr && field != nullptr) {
            auto* value = static_cast<std::string*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = "Center Ice";
            HK_CHECK_EQ(entity.GetComponent<RinkComponent>().rinkName, std::string("Center Ice"));
        }
    }

    // --- vec3 field edit (Transform.Position) --------------------------------
    {
        const ComponentMetadata* md = registry.FindByName("TransformComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "Position") : nullptr;
        HK_CHECK_MSG(field != nullptr, "Position field present");
        if (md != nullptr && field != nullptr) {
            auto* value = static_cast<glm::vec3*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = glm::vec3(1.0f, 2.0f, 3.0f);
            const glm::vec3& pos = entity.GetComponent<TransformComponent>().localPosition;
            HK_CHECK_NEAR(pos.x, 1.0f, 1e-5);
            HK_CHECK_NEAR(pos.y, 2.0f, 1e-5);
            HK_CHECK_NEAR(pos.z, 3.0f, 1e-5);
        }
    }

    // --- enum field edit (Team) ---------------------------------------------
    {
        entity.AddComponent<TeamComponent>();
        const ComponentMetadata* md = registry.FindByName("TeamComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "Team") : nullptr;
        HK_CHECK_MSG(field != nullptr, "Team field present");
        if (md != nullptr && field != nullptr) {
            int* value = static_cast<int*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = static_cast<int>(Team::Away);
            HK_CHECK_MSG(entity.GetComponent<TeamComponent>().team == Team::Away, "enum field written via offset");
        }
    }

    // --- UUID field is read-only --------------------------------------------
    {
        const ComponentMetadata* md = registry.FindByName("IDComponent");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "Id") : nullptr;
        HK_CHECK_MSG(field != nullptr, "UUID field present");
        if (field != nullptr) {
            HK_CHECK_MSG(field->readOnly, "UUID field is read-only");
            HK_CHECK_MSG(field->type == FieldType::UUID, "UUID field typed as UUID");
        }
    }

    // --- vec2 / vec4 field access (synthetic component) ----------------------
    {
        VecProbe probe;
        FieldMetadata two;
        two.type = FieldType::Vec2;
        two.offset = offsetof(VecProbe, two);
        FieldMetadata four;
        four.type = FieldType::Vec4;
        four.offset = offsetof(VecProbe, four);

        auto* twoPtr = static_cast<glm::vec2*>(FieldDrawers::FieldPointer(&probe, two));
        *twoPtr = glm::vec2(4.0f, 5.0f);
        HK_CHECK_NEAR(probe.two.x, 4.0f, 1e-5);
        HK_CHECK_NEAR(probe.two.y, 5.0f, 1e-5);

        auto* fourPtr = static_cast<glm::vec4*>(FieldDrawers::FieldPointer(&probe, four));
        *fourPtr = glm::vec4(1.0f, 2.0f, 3.0f, 4.0f);
        HK_CHECK_NEAR(probe.four.w, 4.0f, 1e-5);
    }

    // --- slider exact-value entry contract ----------------------------------
    {
        const std::string fieldDrawers = ReadProjectFile("libs/editor/src/Inspector/FieldDrawers.cpp");
        HK_CHECK_MSG(Contains(fieldDrawers, "DrawRangedFloatControl"),
                     "ranged metadata float fields use the shared exact-value slider/input control");
        HK_CHECK_MSG(Contains(fieldDrawers, "DrawRangedIntControl"),
                     "ranged metadata int fields use the shared exact-value slider/input control");
        HK_CHECK_MSG(Contains(fieldDrawers, "ImGui::InputFloat"),
                     "ranged metadata float sliders expose numeric text input");
        HK_CHECK_MSG(Contains(fieldDrawers, "ImGui::InputFloat3"),
                     "ranged metadata vec3 sliders expose exact XYZ text input");
        HK_CHECK_MSG(Contains(fieldDrawers, "ImGui::InputInt"),
                     "ranged metadata int sliders expose numeric text input");
        HK_CHECK_MSG(Contains(fieldDrawers, "std::clamp"),
                     "typed slider values are clamped to metadata min/max ranges");
    }
}

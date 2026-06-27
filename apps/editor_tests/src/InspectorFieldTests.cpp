#include "Test.hpp"

#include <filesystem>
#include <string>

#include <glm/glm.hpp>

#include "Hockey/Core/UUID.hpp"
#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/RenderComponents.hpp"
#include "Hockey/ECS/Scene.hpp"
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

    // --- UUID field edit (StickAttachment.stickEntityId) --------------------
    {
        StickAttachmentComponent attachment;
        attachment.stickEntityId = UUID(42ULL);
        entity.AddOrReplaceComponent<StickAttachmentComponent>(attachment);

        const ComponentMetadata* md = registry.FindByName("StickAttachmentComponent");
        HK_CHECK_MSG(md != nullptr, "StickAttachment metadata registered");
        const FieldMetadata* field = md != nullptr ? FindField(*md, "StickEntity") : nullptr;
        HK_CHECK_MSG(field != nullptr, "StickEntity field present");
        if (md != nullptr && field != nullptr) {
            HK_CHECK(field->type == FieldType::UUID);
            auto* value = static_cast<UUID*>(FieldDrawers::FieldPointer(md->getData(entity), *field));
            *value = UUID(43ULL);
            HK_CHECK_EQ(entity.GetComponent<StickAttachmentComponent>().stickEntityId, UUID(43ULL));
        }
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
}

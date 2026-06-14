#include "Hockey/ECS/Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"

namespace Hockey {

glm::mat4 ComposeTransform(const glm::vec3& position, const glm::vec3& rotationDegrees, const glm::vec3& scale) {
    const glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    const glm::mat4 rotation = glm::mat4_cast(glm::quat(glm::radians(rotationDegrees)));
    const glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);
    return translation * rotation * scaling;
}

void DecomposeTransform(const glm::mat4& matrix, glm::vec3& position, glm::vec3& rotationDegrees, glm::vec3& scale) {
    position = glm::vec3(matrix[3]);

    const glm::vec3 column0(matrix[0]);
    const glm::vec3 column1(matrix[1]);
    const glm::vec3 column2(matrix[2]);

    scale.x = glm::length(column0);
    scale.y = glm::length(column1);
    scale.z = glm::length(column2);

    constexpr float kEpsilon = 1e-6f;
    glm::mat3 rotation(1.0f);
    rotation[0] = scale.x > kEpsilon ? column0 / scale.x : glm::vec3(1.0f, 0.0f, 0.0f);
    rotation[1] = scale.y > kEpsilon ? column1 / scale.y : glm::vec3(0.0f, 1.0f, 0.0f);
    rotation[2] = scale.z > kEpsilon ? column2 / scale.z : glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::quat orientation = glm::quat_cast(rotation);
    rotationDegrees = glm::degrees(glm::eulerAngles(orientation));
}

glm::mat4 TransformComponent::LocalMatrix() const {
    return ComposeTransform(localPosition, localRotation, localScale);
}

glm::mat4 Scene::WorldMatrix(entt::entity handle) const {
    const glm::mat4 local = m_Registry.get<TransformComponent>(handle).LocalMatrix();
    if (const auto* parent = m_Registry.try_get<ParentComponent>(handle)) {
        const entt::entity parentHandle = HandleFromUUID(parent->parentId);
        if (parentHandle != entt::null) {
            return WorldMatrix(parentHandle) * local;
        }
    }
    return local;
}

glm::mat4 Scene::GetLocalTransform(Entity entity) const {
    return m_Registry.get<TransformComponent>(entity.Handle()).LocalMatrix();
}

glm::mat4 Scene::GetWorldTransform(Entity entity) const {
    return WorldMatrix(entity.Handle());
}

void Scene::SetWorldTransform(Entity entity, const glm::mat4& worldTransform) {
    glm::mat4 local = worldTransform;
    if (const auto* parent = m_Registry.try_get<ParentComponent>(entity.Handle())) {
        const entt::entity parentHandle = HandleFromUUID(parent->parentId);
        if (parentHandle != entt::null) {
            local = glm::inverse(WorldMatrix(parentHandle)) * worldTransform;
        }
    }

    auto& transform = m_Registry.get<TransformComponent>(entity.Handle());
    DecomposeTransform(local, transform.localPosition, transform.localRotation, transform.localScale);
}

} // namespace Hockey

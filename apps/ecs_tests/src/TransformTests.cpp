#include "Test.hpp"

#include <glm/glm.hpp>

#include "Hockey/ECS/Components.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/Scene.hpp"
#include "Hockey/ECS/Transform.hpp"

using namespace Hockey;

namespace {

bool MatNear(const glm::mat4& a, const glm::mat4& b, float tol = 1e-3f) {
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            if (std::fabs(a[col][row] - b[col][row]) > tol) {
                return false;
            }
        }
    }
    return true;
}

glm::vec3 Translation(const glm::mat4& m) {
    return glm::vec3(m[3]);
}

} // namespace

void RunTransformTests() {
    HockeyTest::BeginSuite("TransformTests");

    // Compose: pure translation.
    glm::mat4 t = ComposeTransform(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f), glm::vec3(1.0f));
    HK_CHECK_NEAR(Translation(t).x, 1.0f, 1e-4);
    HK_CHECK_NEAR(Translation(t).y, 2.0f, 1e-4);
    HK_CHECK_NEAR(Translation(t).z, 3.0f, 1e-4);

    // Decompose round-trips back to the same matrix.
    glm::mat4 original =
        ComposeTransform(glm::vec3(4.0f, -2.0f, 8.0f), glm::vec3(30.0f, 45.0f, -15.0f), glm::vec3(2.0f, 3.0f, 1.5f));
    glm::vec3 pos, rot, scale;
    DecomposeTransform(original, pos, rot, scale);
    glm::mat4 recomposed = ComposeTransform(pos, rot, scale);
    HK_CHECK(MatNear(original, recomposed));

    // LocalMatrix reflects component values.
    TransformComponent tc;
    tc.localPosition = glm::vec3(7.0f, 0.0f, 0.0f);
    HK_CHECK_NEAR(Translation(tc.LocalMatrix()).x, 7.0f, 1e-4);

    // Parent transform affects child world transform.
    Scene scene("Test");
    Entity parent = scene.CreateEntity("Parent");
    Entity child = scene.CreateEntity("Child");
    parent.GetComponent<TransformComponent>().localPosition = glm::vec3(10.0f, 0.0f, 0.0f);
    child.GetComponent<TransformComponent>().localPosition = glm::vec3(1.0f, 0.0f, 0.0f);
    scene.SetParent(child, parent, false);
    HK_CHECK_NEAR(Translation(scene.GetWorldTransform(child)).x, 11.0f, 1e-3);

    // Reparent with keepWorldTransform preserves the world transform.
    Entity a = scene.CreateEntity("A");
    Entity b = scene.CreateEntity("B");
    Entity mover = scene.CreateEntity("Mover");
    a.GetComponent<TransformComponent>().localPosition = glm::vec3(10.0f, 0.0f, 0.0f);
    b.GetComponent<TransformComponent>().localPosition = glm::vec3(0.0f, 5.0f, -3.0f);
    mover.GetComponent<TransformComponent>().localPosition = glm::vec3(2.0f, 0.0f, 0.0f);
    scene.SetParent(mover, a, true);
    glm::mat4 worldBefore = scene.GetWorldTransform(mover);
    scene.SetParent(mover, b, true);
    glm::mat4 worldAfter = scene.GetWorldTransform(mover);
    HK_CHECK(MatNear(worldBefore, worldAfter));

    // Unparent with keepWorldTransform preserves the world transform.
    glm::mat4 worldBeforeUnparent = scene.GetWorldTransform(mover);
    scene.RemoveParent(mover, true);
    glm::mat4 worldAfterUnparent = scene.GetWorldTransform(mover);
    HK_CHECK(MatNear(worldBeforeUnparent, worldAfterUnparent));
    HK_CHECK(!mover.HasComponent<ParentComponent>());
}

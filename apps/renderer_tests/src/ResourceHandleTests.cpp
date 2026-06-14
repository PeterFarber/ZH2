#include "Test.hpp"

#include <unordered_map>

#include "Hockey/Renderer/Material.hpp"
#include "Hockey/Renderer/Mesh.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"

using namespace Hockey;

void RunResourceHandleTests() {
    HockeyTest::BeginSuite("ResourceHandleTests");

    // Default-constructed handles are invalid (id == 0).
    HK_CHECK(!MeshHandle{}.IsValid());
    HK_CHECK(!TextureHandle{}.IsValid());
    HK_CHECK(!MaterialHandle{}.IsValid());
    HK_CHECK(!ShaderHandle{}.IsValid());
    HK_CHECK(!PipelineHandle{}.IsValid());
    HK_CHECK(MeshHandle{}.id == 0);

    // Non-zero ids are valid.
    HK_CHECK(MeshHandle{1}.IsValid());
    HK_CHECK(TextureHandle{42}.IsValid());

    // Equality / inequality.
    HK_CHECK(MeshHandle{7} == MeshHandle{7});
    HK_CHECK(MeshHandle{7} != MeshHandle{8});

    // Usable as a hash-map key.
    std::unordered_map<TextureHandle, int> map;
    map[TextureHandle{3}] = 30;
    map[TextureHandle{4}] = 40;
    HK_CHECK(map[TextureHandle{3}] == 30);
    HK_CHECK(map[TextureHandle{4}] == 40);
    HK_CHECK(map.size() == 2);

    // Built-in mesh generators produce non-degenerate, index-bounded geometry.
    for (int i = 0; i < 8; ++i) {
        const MeshDesc mesh = MakeBuiltInMesh(static_cast<BuiltInMesh>(i));
        HK_CHECK_MSG(!mesh.vertices.empty(), BuiltInMeshName(static_cast<BuiltInMesh>(i)));
        HK_CHECK_MSG(!mesh.indices.empty(), BuiltInMeshName(static_cast<BuiltInMesh>(i)));
        HK_CHECK(mesh.indices.size() % 3 == 0);
        uint32_t maxIndex = 0;
        for (uint32_t index : mesh.indices) {
            maxIndex = index > maxIndex ? index : maxIndex;
        }
        HK_CHECK(maxIndex < mesh.vertices.size());
    }

    // Built-in materials produce stable names.
    for (int i = 0; i < 11; ++i) {
        const auto material = static_cast<BuiltInMaterial>(i);
        const MaterialDesc desc = MakeBuiltInMaterial(material);
        HK_CHECK(desc.debugName == BuiltInMaterialName(material));
    }
}

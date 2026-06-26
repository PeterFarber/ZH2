#include "Test.hpp"

#include "Hockey/Core/Paths.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace {

std::string ReadProjectFile(const char* relativePath) {
    std::ifstream in(Hockey::Paths::Get().root / relativePath, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

bool Contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

void RequireMaterialFile(const char* relativePath, const char* materialName) {
    const std::filesystem::path path = Hockey::Paths::Get().root / relativePath;
    HK_CHECK_MSG(std::filesystem::exists(path), relativePath);
    const std::string text = ReadProjectFile(relativePath);
    HK_CHECK_MSG(Contains(text, "Material:"), relativePath);
    HK_CHECK_MSG(Contains(text, "Type: PBR"), relativePath);
    HK_CHECK_MSG(Contains(text, materialName), materialName);
}

} // namespace

void RunMaterialAssetContractTests() {
    HockeyTest::BeginSuite("MaterialAssetContractTests");

    RequireMaterialFile("data/raw/materials/DefaultWhite.material.yaml", "DefaultWhite");
    RequireMaterialFile("data/raw/materials/ErrorMagenta.material.yaml", "ErrorMagenta");
    RequireMaterialFile("data/raw/materials/Ice.material.yaml", "Ice");
    RequireMaterialFile("data/raw/materials/PuckBlack.material.yaml", "PuckBlack");
    RequireMaterialFile("data/raw/materials/HomeJersey.material.yaml", "HomeJersey");
    RequireMaterialFile("data/raw/materials/AwayJersey.material.yaml", "AwayJersey");
    RequireMaterialFile("data/raw/materials/Boards.material.yaml", "Boards");
    RequireMaterialFile("data/raw/materials/Glass.material.yaml", "Glass");
    RequireMaterialFile("data/raw/materials/GoalNet.material.yaml", "GoalNet");
    RequireMaterialFile("data/raw/materials/DebugCollider.material.yaml", "DebugCollider");
    RequireMaterialFile("data/raw/materials/DebugTrigger.material.yaml", "DebugTrigger");
    RequireMaterialFile("data/raw/materials/Metal.material.yaml", "Metal");

    const std::string materialHeader = ReadProjectFile("libs/renderer/include/Hockey/Renderer/Material.hpp");
    const std::string rendererHeader = ReadProjectFile("libs/renderer/include/Hockey/Renderer/Renderer.hpp");
    const std::string rendererSource = ReadProjectFile("libs/renderer/src/Renderer.cpp");
    const std::string ecsHeader = ReadProjectFile("libs/ecs/include/Hockey/ECS/RenderComponents.hpp");
    const std::string pickerSource = ReadProjectFile("libs/editor/src/Inspector/MaterialPicker.cpp");

    HK_CHECK_MSG(!Contains(materialHeader, "BuiltInMaterial"), "renderer exposes no BuiltInMaterial enum");
    HK_CHECK_MSG(!Contains(materialHeader, "MakeBuiltInMaterial"), "renderer exposes no material factory");
    HK_CHECK_MSG(!Contains(rendererHeader, "GetBuiltInMaterial"), "renderer exposes no built-in material getter");
    HK_CHECK_MSG(!Contains(rendererSource, "ParseBuiltInMaterial"), "renderer parses no built-in material names");
    HK_CHECK_MSG(!Contains(rendererSource, "builtInMaterials"), "renderer owns no built-in material table");
    HK_CHECK_MSG(!Contains(ecsHeader, "materialName"), "ECS render components have no materialName fallback");
    HK_CHECK_MSG(!Contains(pickerSource, "Built-in"), "editor material picker has no built-in material section");
}

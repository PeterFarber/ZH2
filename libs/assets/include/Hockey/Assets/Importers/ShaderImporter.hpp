#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetID.hpp"
#include "Hockey/Assets/Importer.hpp"

namespace Hockey {

class AssetDatabase;

// Imports GLSL shader sources (.vert/.frag/.comp and .glsl include libraries).
// Records #include dependencies; compilation to SPIR-V is the cooker's job.
class ShaderImporter : public Importer {
public:
    bool SupportsExtension(const std::string& extension) const override;
    AssetType GetAssetType() const override {
        return AssetType::Shader;
    }
    std::string Version() const override {
        return "1.0.0";
    }
    ImportResult Import(const ImportContext& context) override;

    // Scans `#include "..."` directives in the shader source and resolves each to
    // a Shader AssetID (relative to the shader's directory). Shared by importer
    // and cooker so dependency identity is consistent.
    static std::vector<AssetID> ScanIncludeDependencies(const std::filesystem::path& shaderAbsolute,
                                                        const std::filesystem::path& projectRoot,
                                                        AssetDatabase& database);
};

} // namespace Hockey

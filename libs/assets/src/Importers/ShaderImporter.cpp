#include "Hockey/Assets/Importers/ShaderImporter.hpp"

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetPath.hpp"
#include "Hockey/Assets/Assets/ShaderAsset.hpp"
#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace Hockey {
namespace fs = std::filesystem;

namespace {

// Extracts the path from a `#include "path"` (or <path>) line, or "" if the line
// is not an include directive.
std::string ParseIncludePath(const std::string& line) {
    size_t i = line.find_first_not_of(" \t");
    if (i == std::string::npos || line[i] != '#') {
        return {};
    }
    ++i;
    i = line.find_first_not_of(" \t", i);
    static const std::string kInclude = "include";
    if (i == std::string::npos || line.compare(i, kInclude.size(), kInclude) != 0) {
        return {};
    }
    i += kInclude.size();
    const size_t open = line.find_first_of("\"<", i);
    if (open == std::string::npos) {
        return {};
    }
    const char closeChar = line[open] == '"' ? '"' : '>';
    const size_t close = line.find(closeChar, open + 1);
    if (close == std::string::npos) {
        return {};
    }
    return line.substr(open + 1, close - open - 1);
}

} // namespace

bool ShaderImporter::SupportsExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".glsl";
}

std::vector<AssetID> ShaderImporter::ScanIncludeDependencies(const fs::path& shaderAbsolute,
                                                             const fs::path& projectRoot, AssetDatabase& database) {
    std::vector<AssetID> deps;
    const Result<std::string> source = FileSystem::ReadTextFile(shaderAbsolute);
    if (!source) {
        return deps;
    }

    std::istringstream stream(source.value);
    std::string line;
    while (std::getline(stream, line)) {
        const std::string included = ParseIncludePath(line);
        if (included.empty()) {
            continue;
        }
        const fs::path includeAbsolute = shaderAbsolute.parent_path() / included;
        if (!FileSystem::Exists(includeAbsolute)) {
            continue;
        }
        const fs::path includeRel = AssetPath::ToProjectRelative(projectRoot, includeAbsolute);
        const AssetID id = database.GetOrCreateIDForPath(includeRel, AssetType::Shader);
        if (id.IsValid() && std::find(deps.begin(), deps.end(), id) == deps.end()) {
            deps.push_back(id);
        }
    }
    return deps;
}

ImportResult ShaderImporter::Import(const ImportContext& context) {
    ImportResult result;
    if (!FileSystem::Exists(context.rawPath)) {
        result.success = false;
        result.error = "shader source not found: " + context.rawPath.string();
        return result;
    }

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Shader;
    result.metadata.name = context.rawPath.filename().string();

    if (context.database != nullptr) {
        result.metadata.dependencies = ScanIncludeDependencies(context.rawPath, context.projectRoot, *context.database);
    }
    return result;
}

} // namespace Hockey

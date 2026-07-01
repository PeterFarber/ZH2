#include "Hockey/Assets/Importers/AudioImporter.hpp"

#include "Hockey/Core/FileSystem.hpp"

#include <algorithm>

namespace Hockey {

bool AudioImporter::SupportsExtension(const std::string& extension) const {
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".mp3" || ext == ".wav" || ext == ".flac";
}

ImportResult AudioImporter::Import(const ImportContext& context) {
    ImportResult result;
    if (!FileSystem::Exists(context.rawPath)) {
        result.success = false;
        result.error = "audio source not found: " + context.rawPath.string();
        return result;
    }

    result.success = true;
    result.metadata.id = context.existingId;
    result.metadata.type = AssetType::Audio;
    result.metadata.name = context.rawPath.filename().string();
    return result;
}

} // namespace Hockey

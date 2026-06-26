#pragma once

#include <filesystem>

#include <RmlUi/Core/FileInterface.h>

namespace Hockey {

class RmlUiFileInterface final : public Rml::FileInterface {
public:
    explicit RmlUiFileInterface(std::filesystem::path root);

    Rml::FileHandle Open(const Rml::String& path) override;
    void Close(Rml::FileHandle file) override;
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;
    bool Seek(Rml::FileHandle file, long offset, int origin) override;
    size_t Tell(Rml::FileHandle file) override;

private:
    std::filesystem::path ResolveUiPath(const Rml::String& path) const;

    std::filesystem::path m_Root;
};

} // namespace Hockey

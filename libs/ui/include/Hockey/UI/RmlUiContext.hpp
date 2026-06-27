#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <RmlUi/Core/Types.h>

#include "Hockey/UI/UISettings.hpp"

namespace Rml {
class Context;
class ElementDocument;
} // namespace Rml

namespace Hockey {

class Renderer;
class RmlUiFileInterface;
class RmlUiRenderInterface;
class RmlUiSystemInterface;
class RmlUiClickListener;

struct RmlUiContextDesc {
    Renderer* renderer = nullptr;
    std::filesystem::path root;
    UISettings settings = MakeDefaultUISettings();
    std::string name = "hockey-ui";
    uint32_t width = 1280;
    uint32_t height = 720;
};

class RmlUiContext {
public:
    RmlUiContext();
    ~RmlUiContext();

    RmlUiContext(const RmlUiContext&) = delete;
    RmlUiContext& operator=(const RmlUiContext&) = delete;

    bool Initialize(const RmlUiContextDesc& desc);
    void Shutdown();
    bool IsInitialized() const;

    void SetDimensions(uint32_t width, uint32_t height);
    bool LoadDocument(const std::string& documentPath);
    bool BindClickAction(const std::string& elementId, std::function<void()> action);
    bool SetElementText(const std::string& elementId, const std::string& text);
    void UnloadAllDocuments();
    int LoadedDocumentCount() const;

    void Update();
    void Render();
    bool ProcessMouseMove(int x, int y);
    bool ProcessMouseButton(int button, bool pressed);
    bool ProcessMouseWheel(float x, float y);

    Rml::Context* RawContext() const;
    RmlUiRenderInterface* RenderInterface();

private:
    Rml::Context* m_Context = nullptr;
    std::string m_ContextName;
    UISettings m_Settings = MakeDefaultUISettings();
    RmlUiSystemInterface* m_SystemInterface = nullptr;
    RmlUiFileInterface* m_FileInterface = nullptr;
    RmlUiRenderInterface* m_RenderInterface = nullptr;
    std::vector<std::unique_ptr<RmlUiClickListener>> m_ClickListeners;
};

} // namespace Hockey

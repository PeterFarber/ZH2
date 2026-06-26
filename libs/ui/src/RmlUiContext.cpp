#include "Hockey/UI/RmlUiContext.hpp"

#include <memory>
#include <utility>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/EventListener.h>

#include "Hockey/UI/RmlUiFileInterface.hpp"
#include "Hockey/UI/RmlUiRenderInterface.hpp"
#include "Hockey/UI/RmlUiSystemInterface.hpp"

namespace Hockey {
namespace {

bool g_RmlUiInitialized = false;
std::unique_ptr<RmlUiSystemInterface> g_SystemInterface;
std::unique_ptr<RmlUiFileInterface> g_FileInterface;
std::unique_ptr<RmlUiRenderInterface> g_RenderInterface;

} // namespace

class RmlUiClickListener final : public Rml::EventListener {
public:
    explicit RmlUiClickListener(std::function<void()> action) : m_Action(std::move(action)) {}

    void ProcessEvent(Rml::Event& event) override {
        (void)event;
        if (m_Action) {
            m_Action();
        }
    }

private:
    std::function<void()> m_Action;
};

RmlUiContext::RmlUiContext() = default;

RmlUiContext::~RmlUiContext() {
    Shutdown();
}

bool RmlUiContext::Initialize(const RmlUiContextDesc& desc) {
    if (IsInitialized()) {
        return true;
    }
    if (g_RmlUiInitialized || desc.renderer == nullptr || desc.root.empty() || desc.width == 0 || desc.height == 0) {
        return false;
    }

    m_Settings = desc.settings;
    m_ContextName = desc.name.empty() ? "hockey-ui" : desc.name;

    g_SystemInterface = std::make_unique<RmlUiSystemInterface>();
    g_FileInterface = std::make_unique<RmlUiFileInterface>(desc.root);
    g_RenderInterface = std::make_unique<RmlUiRenderInterface>(*desc.renderer);

    m_SystemInterface = g_SystemInterface.get();
    m_FileInterface = g_FileInterface.get();
    m_RenderInterface = g_RenderInterface.get();

    Rml::SetSystemInterface(m_SystemInterface);
    Rml::SetFileInterface(m_FileInterface);
    Rml::SetRenderInterface(m_RenderInterface);

    if (!Rml::Initialise()) {
        m_SystemInterface = nullptr;
        m_FileInterface = nullptr;
        m_RenderInterface = nullptr;
        g_RenderInterface.reset();
        g_FileInterface.reset();
        g_SystemInterface.reset();
        return false;
    }
    g_RmlUiInitialized = true;

    if (!m_Settings.defaultFont.empty() && !Rml::LoadFontFace(m_Settings.defaultFont, true)) {
        Shutdown();
        return false;
    }

    m_Context = Rml::CreateContext(m_ContextName, Rml::Vector2i{static_cast<int>(desc.width), static_cast<int>(desc.height)},
                                   m_RenderInterface);
    if (m_Context == nullptr) {
        Shutdown();
        return false;
    }
    m_Context->SetDensityIndependentPixelRatio(m_Settings.uiScale);
    return true;
}

void RmlUiContext::Shutdown() {
    if (!g_RmlUiInitialized && m_Context == nullptr) {
        return;
    }

    if (m_Context != nullptr) {
        m_Context->UnloadAllDocuments();
        Rml::RemoveContext(m_ContextName);
        m_Context = nullptr;
        m_ClickListeners.clear();
    }

    if (g_RmlUiInitialized) {
        Rml::Shutdown();
        g_RmlUiInitialized = false;
    }

    m_SystemInterface = nullptr;
    m_FileInterface = nullptr;
    m_RenderInterface = nullptr;
    g_RenderInterface.reset();
    g_FileInterface.reset();
    g_SystemInterface.reset();
}

bool RmlUiContext::IsInitialized() const {
    return m_Context != nullptr;
}

void RmlUiContext::SetDimensions(uint32_t width, uint32_t height) {
    if (m_Context == nullptr || width == 0 || height == 0) {
        return;
    }
    m_Context->SetDimensions(Rml::Vector2i{static_cast<int>(width), static_cast<int>(height)});
}

bool RmlUiContext::LoadDocument(const std::string& documentPath) {
    if (m_Context == nullptr) {
        return false;
    }

    Rml::ElementDocument* document = m_Context->LoadDocument(documentPath);
    if (document == nullptr) {
        return false;
    }
    document->Show();
    return true;
}

bool RmlUiContext::BindClickAction(const std::string& elementId, std::function<void()> action) {
    if (m_Context == nullptr || elementId.empty() || !action) {
        return false;
    }

    for (int i = 0; i < m_Context->GetNumDocuments(); ++i) {
        Rml::ElementDocument* document = m_Context->GetDocument(i);
        if (document == nullptr) {
            continue;
        }
        Rml::Element* element = document->GetElementById(elementId);
        if (element == nullptr) {
            continue;
        }

        auto listener = std::make_unique<RmlUiClickListener>(std::move(action));
        element->AddEventListener("click", listener.get());
        m_ClickListeners.push_back(std::move(listener));
        return true;
    }
    return false;
}

void RmlUiContext::UnloadAllDocuments() {
    if (m_Context != nullptr) {
        m_Context->UnloadAllDocuments();
    }
}

int RmlUiContext::LoadedDocumentCount() const {
    return m_Context != nullptr ? m_Context->GetNumDocuments() : 0;
}

void RmlUiContext::Update() {
    if (m_Context != nullptr) {
        m_Context->Update();
    }
}

void RmlUiContext::Render() {
    if (m_Context != nullptr && m_RenderInterface != nullptr) {
        m_RenderInterface->ClearDrawCommands();
        m_Context->Render();
    }
}

Rml::Context* RmlUiContext::RawContext() const {
    return m_Context;
}

RmlUiRenderInterface* RmlUiContext::RenderInterface() {
    return m_RenderInterface;
}

} // namespace Hockey

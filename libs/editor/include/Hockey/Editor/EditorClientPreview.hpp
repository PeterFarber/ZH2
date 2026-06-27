#pragma once

#include <cstdint>
#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Renderer/RenderHandles.hpp"
#include "Hockey/UI/ClientFlowRunner.hpp"
#include "Hockey/UI/RmlUiContext.hpp"

namespace Hockey {

class EditorContext;

// Hosts the shipped runtime client flow inside the editor. It uses the same
// RmlUi documents and client-flow asset as HockeyGameClient, but renders into
// an editor offscreen target instead of the swapchain.
class EditorClientPreview {
public:
    Status Start(EditorContext& context, const std::filesystem::path& flowPath = {});
    void Stop(EditorContext& context);
    void Update(EditorContext& context, float deltaTime);
    void RenderToTarget(EditorContext& context, TextureHandle target, std::uint32_t width, std::uint32_t height);
    void HandlePointerInput(int x, int y, bool leftPressed, bool leftReleased, float wheelY);

    bool IsActive() const {
        return m_Active;
    }

    bool IsRunning() const {
        return m_Active && m_ClientFlow.ActiveScreen() == UIScreenId::MatchHud;
    }

    UIScreenId ActiveScreen() const {
        return m_ClientFlow.ActiveScreen();
    }

private:
    bool LoadActiveScreen();
    void BindRuntimeActions();
    void QueueAction(UIAction action);
    void ApplyHud(EditorContext& context);

    RmlUiContext m_UIContext;
    ClientFlowRunner m_ClientFlow;
    std::filesystem::path m_FlowPath;
    bool m_Active = false;
    bool m_ReloadRequested = false;
    bool m_StartedGameplayPreview = false;
    bool m_RestoreDirty = false;
};

} // namespace Hockey

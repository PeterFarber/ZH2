#pragma once

#include <string>

namespace Hockey {

class EditorContext;

// Base class for every editor dock panel. Each panel is a single ImGui window
// whose title doubles as its unique name and as the dock target used by the
// dockspace's default layout. The PanelManager owns panels and drives their
// per-frame OnUpdate/OnImGui.
class Panel {
public:
    explicit Panel(std::string name, bool openByDefault = true);
    virtual ~Panel() = default;

    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    const std::string& GetName() const;

    bool IsOpen() const;
    void SetOpen(bool open);

    virtual void OnUpdate(EditorContext& context, float deltaTime);
    virtual void OnImGui(EditorContext& context) = 0;

protected:
    // Opens this panel's ImGui window, wired to the close button and the open
    // flag. Returns true when the body is visible. Always pair with EndWindow().
    // windowFlags is an ImGuiWindowFlags value (kept as int so this header stays
    // free of the ImGui include).
    bool BeginWindow(int windowFlags = 0);
    void EndWindow();

private:
    std::string m_Name;
    bool m_Open = true;
};

} // namespace Hockey

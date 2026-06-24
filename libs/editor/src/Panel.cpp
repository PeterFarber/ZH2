#include "Hockey/Editor/Panel.hpp"

#include <utility>

#include <imgui.h>

namespace Hockey {

Panel::Panel(std::string name, bool openByDefault)
    : m_Name(std::move(name)), m_Open(openByDefault), m_OpenByDefault(openByDefault) {}

const std::string& Panel::GetName() const {
    return m_Name;
}

bool Panel::IsOpen() const {
    return m_Open;
}

bool Panel::IsOpenByDefault() const {
    return m_OpenByDefault;
}

void Panel::SetOpen(bool open) {
    m_Open = open;
}

void Panel::ResetOpenState() {
    m_Open = m_OpenByDefault;
}

void Panel::OnUpdate(EditorContext& /*context*/, float /*deltaTime*/) {}

bool Panel::BeginWindow(int windowFlags) {
    return ImGui::Begin(m_Name.c_str(), &m_Open, static_cast<ImGuiWindowFlags>(windowFlags));
}

void Panel::EndWindow() {
    ImGui::End();
}

} // namespace Hockey

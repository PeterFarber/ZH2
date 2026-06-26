#include "Hockey/UI/UIInputMapper.hpp"

#include <cmath>

#include <RmlUi/Core/Context.h>

namespace Hockey {
namespace {

bool IsCapturedByRml(bool rmlResult, const Rml::Context& context) {
    return !rmlResult || context.GetNumDocuments() > 0;
}

int NoModifiers() {
    return 0;
}

} // namespace

bool UIInputMapper::ProcessEvent(Rml::Context& context, const Event& event) {
    switch (event.type) {
    case EventType::MouseMoved: {
        const bool result =
            context.ProcessMouseMove(static_cast<int>(std::lround(event.mouseX)), static_cast<int>(std::lround(event.mouseY)),
                                     NoModifiers());
        m_WantsMouseCapture = IsCapturedByRml(result, context);
        return m_WantsMouseCapture;
    }
    case EventType::MouseButtonPressed: {
        const bool result =
            context.ProcessMouseButtonDown(MapMouseButton(Mouse::FromSDLButton(event.mouseButton)), NoModifiers());
        m_WantsMouseCapture = IsCapturedByRml(result, context);
        return m_WantsMouseCapture;
    }
    case EventType::MouseButtonReleased: {
        const bool result =
            context.ProcessMouseButtonUp(MapMouseButton(Mouse::FromSDLButton(event.mouseButton)), NoModifiers());
        m_WantsMouseCapture = IsCapturedByRml(result, context);
        return m_WantsMouseCapture;
    }
    case EventType::MouseScrolled: {
        const bool result = context.ProcessMouseWheel(Rml::Vector2f{event.scrollX, event.scrollY}, NoModifiers());
        m_WantsMouseCapture = IsCapturedByRml(result, context);
        return m_WantsMouseCapture;
    }
    case EventType::KeyPressed: {
        const Rml::Input::KeyIdentifier key = MapKey(Keyboard::FromScancode(event.key));
        if (key == Rml::Input::KI_UNKNOWN) {
            return false;
        }
        const bool result = context.ProcessKeyDown(key, NoModifiers());
        m_WantsKeyboardCapture = IsCapturedByRml(result, context);
        return m_WantsKeyboardCapture;
    }
    case EventType::KeyReleased: {
        const Rml::Input::KeyIdentifier key = MapKey(Keyboard::FromScancode(event.key));
        if (key == Rml::Input::KI_UNKNOWN) {
            return false;
        }
        const bool result = context.ProcessKeyUp(key, NoModifiers());
        m_WantsKeyboardCapture = IsCapturedByRml(result, context);
        return m_WantsKeyboardCapture;
    }
    case EventType::TextInput: {
        if (event.text.empty()) {
            return false;
        }
        const bool result = context.ProcessTextInput(event.text);
        m_WantsKeyboardCapture = IsCapturedByRml(result, context);
        m_WantsTextInput = m_WantsKeyboardCapture;
        return m_WantsKeyboardCapture;
    }
    default:
        return false;
    }
}

bool UIInputMapper::WantsMouseCapture() const {
    return m_WantsMouseCapture;
}

bool UIInputMapper::WantsKeyboardCapture() const {
    return m_WantsKeyboardCapture;
}

bool UIInputMapper::WantsTextInput() const {
    return m_WantsTextInput;
}

void UIInputMapper::ClearCapture() {
    m_WantsMouseCapture = false;
    m_WantsKeyboardCapture = false;
    m_WantsTextInput = false;
}

Rml::Input::KeyIdentifier UIInputMapper::MapKey(KeyCode key) {
    switch (key) {
    case KeyCode::Escape:
        return Rml::Input::KI_ESCAPE;
    case KeyCode::Space:
        return Rml::Input::KI_SPACE;
    case KeyCode::W:
        return Rml::Input::KI_W;
    case KeyCode::A:
        return Rml::Input::KI_A;
    case KeyCode::S:
        return Rml::Input::KI_S;
    case KeyCode::D:
        return Rml::Input::KI_D;
    case KeyCode::Z:
        return Rml::Input::KI_Z;
    case KeyCode::X:
        return Rml::Input::KI_X;
    case KeyCode::R:
        return Rml::Input::KI_R;
    case KeyCode::F12:
        return Rml::Input::KI_F12;
    case KeyCode::Up:
        return Rml::Input::KI_UP;
    case KeyCode::Down:
        return Rml::Input::KI_DOWN;
    case KeyCode::Left:
        return Rml::Input::KI_LEFT;
    case KeyCode::Right:
        return Rml::Input::KI_RIGHT;
    case KeyCode::Unknown:
    default:
        return Rml::Input::KI_UNKNOWN;
    }
}

int UIInputMapper::MapMouseButton(MouseButton button) {
    switch (button) {
    case MouseButton::Left:
        return 0;
    case MouseButton::Right:
        return 1;
    case MouseButton::Middle:
        return 2;
    }
    return 0;
}

} // namespace Hockey

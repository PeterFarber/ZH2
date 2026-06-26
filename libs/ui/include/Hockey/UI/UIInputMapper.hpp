#pragma once

#include <cstdint>

#include <RmlUi/Core/Input.h>

#include "Hockey/Core/Event.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Mouse.hpp"

namespace Rml {
class Context;
} // namespace Rml

namespace Hockey {

class UIInputMapper {
public:
    bool ProcessEvent(Rml::Context& context, const Event& event);

    bool WantsMouseCapture() const;
    bool WantsKeyboardCapture() const;
    bool WantsTextInput() const;
    void ClearCapture();

    static Rml::Input::KeyIdentifier MapKey(KeyCode key);
    static int MapMouseButton(MouseButton button);

private:
    bool m_WantsMouseCapture = false;
    bool m_WantsKeyboardCapture = false;
    bool m_WantsTextInput = false;
};

} // namespace Hockey

#pragma once
#include <cstdint>
namespace Hockey {
enum class EventType {
    None,
    WindowClose,
    WindowResize,
    WindowMinimized,
    WindowRestored,
    WindowFocusGained,
    WindowFocusLost,
    KeyPressed,
    KeyReleased,
    TextInput,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled,
    GamepadConnected,
    GamepadDisconnected,
    GamepadButtonPressed,
    GamepadButtonReleased,
    GamepadAxisMoved
};
struct Event {
    EventType type = EventType::None;
    uint32_t windowWidth = 0, windowHeight = 0;
    int key = 0;
    bool keyRepeat = false;
    int mouseButton = 0;
    float mouseX = 0, mouseY = 0, mouseDeltaX = 0, mouseDeltaY = 0, scrollX = 0, scrollY = 0;
    int gamepadId = -1, gamepadButton = 0, gamepadAxis = 0;
    float gamepadAxisValue = 0.0f;
};
} // namespace Hockey

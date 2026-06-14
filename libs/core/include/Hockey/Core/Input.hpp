#pragma once
#include <vector>
#include "Hockey/Core/Event.hpp"
#include "Hockey/Core/Gamepad.hpp"
#include "Hockey/Core/Keyboard.hpp"
#include "Hockey/Core/Mouse.hpp"
namespace Hockey {
class Input {
public:
    static void Init();
    static void Shutdown();

    static void BeginFrame();
    static void ProcessEvent(const Event& event);
    static void EndFrame();

    // Keyboard (raw SDL scancode integers, see Keyboard helpers)
    static bool IsKeyDown(int scancode);
    static bool WasKeyPressed(int scancode);
    static bool WasKeyReleased(int scancode);

    static bool IsKeyDown(KeyCode key);
    static bool WasKeyPressed(KeyCode key);
    static bool WasKeyReleased(KeyCode key);

    // Mouse
    static float MouseX();
    static float MouseY();
    static float MouseDeltaX();
    static float MouseDeltaY();
    static float ScrollX();
    static float ScrollY();
    static bool IsMouseButtonDown(MouseButton button);
    static bool WasMouseButtonPressed(MouseButton button);
    static bool WasMouseButtonReleased(MouseButton button);

    // Gamepads
    static std::vector<Gamepad> ConnectedGamepads();
};
}

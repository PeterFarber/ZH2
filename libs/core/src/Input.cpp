#include "Hockey/Core/Input.hpp"
#include <unordered_map>
#include <unordered_set>
#include <SDL3/SDL.h>
namespace Hockey {
namespace {
struct InputState {
    std::unordered_set<int> keysDown;
    std::unordered_set<int> keysPressed;
    std::unordered_set<int> keysReleased;

    std::unordered_set<int> mouseDown;
    std::unordered_set<int> mousePressed;
    std::unordered_set<int> mouseReleased;

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float mouseDeltaX = 0.0f;
    float mouseDeltaY = 0.0f;
    float scrollX = 0.0f;
    float scrollY = 0.0f;

    std::unordered_map<int, SDL_Gamepad*> gamepads;
    bool gamepadEnabled = true;
};
InputState g_State;

void OpenGamepad(int instanceId) {
    if (!g_State.gamepadEnabled) {
        return;
    }
    if (g_State.gamepads.find(instanceId) != g_State.gamepads.end()) {
        return;
    }
    SDL_Gamepad* pad = SDL_OpenGamepad(static_cast<SDL_JoystickID>(instanceId));
    if (pad != nullptr) {
        g_State.gamepads[instanceId] = pad;
    }
}
void CloseGamepad(int instanceId) {
    auto it = g_State.gamepads.find(instanceId);
    if (it != g_State.gamepads.end()) {
        SDL_CloseGamepad(it->second);
        g_State.gamepads.erase(it);
    }
}
}
void Input::Init() {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (ids != nullptr) {
        for (int i = 0; i < count; ++i) {
            OpenGamepad(static_cast<int>(ids[i]));
        }
        SDL_free(ids);
    }
}
void Input::Shutdown() {
    for (auto& entry : g_State.gamepads) {
        SDL_CloseGamepad(entry.second);
    }
    g_State = InputState{};
}
void Input::BeginFrame() {
    g_State.keysPressed.clear();
    g_State.keysReleased.clear();
    g_State.mousePressed.clear();
    g_State.mouseReleased.clear();
    g_State.mouseDeltaX = 0.0f;
    g_State.mouseDeltaY = 0.0f;
    g_State.scrollX = 0.0f;
    g_State.scrollY = 0.0f;
}
void Input::ProcessEvent(const Event& event) {
    switch (event.type) {
        case EventType::KeyPressed:
            if (!event.keyRepeat) {
                g_State.keysPressed.insert(event.key);
            }
            g_State.keysDown.insert(event.key);
            break;
        case EventType::KeyReleased:
            g_State.keysDown.erase(event.key);
            g_State.keysReleased.insert(event.key);
            break;
        case EventType::MouseMoved:
            g_State.mouseX = event.mouseX;
            g_State.mouseY = event.mouseY;
            g_State.mouseDeltaX += event.mouseDeltaX;
            g_State.mouseDeltaY += event.mouseDeltaY;
            break;
        case EventType::MouseButtonPressed:
            g_State.mouseDown.insert(event.mouseButton);
            g_State.mousePressed.insert(event.mouseButton);
            break;
        case EventType::MouseButtonReleased:
            g_State.mouseDown.erase(event.mouseButton);
            g_State.mouseReleased.insert(event.mouseButton);
            break;
        case EventType::MouseScrolled:
            g_State.scrollX += event.scrollX;
            g_State.scrollY += event.scrollY;
            break;
        case EventType::GamepadConnected:
            OpenGamepad(event.gamepadId);
            break;
        case EventType::GamepadDisconnected:
            CloseGamepad(event.gamepadId);
            break;
        default:
            break;
    }
}
void Input::EndFrame() {}
bool Input::IsKeyDown(int scancode) { return g_State.keysDown.count(scancode) > 0; }
bool Input::WasKeyPressed(int scancode) { return g_State.keysPressed.count(scancode) > 0; }
bool Input::WasKeyReleased(int scancode) { return g_State.keysReleased.count(scancode) > 0; }
bool Input::IsKeyDown(KeyCode key) {
    const int scancode = Keyboard::ToScancode(key);
    return scancode >= 0 && IsKeyDown(scancode);
}
bool Input::WasKeyPressed(KeyCode key) {
    const int scancode = Keyboard::ToScancode(key);
    return scancode >= 0 && WasKeyPressed(scancode);
}
bool Input::WasKeyReleased(KeyCode key) {
    const int scancode = Keyboard::ToScancode(key);
    return scancode >= 0 && WasKeyReleased(scancode);
}
float Input::MouseX() { return g_State.mouseX; }
float Input::MouseY() { return g_State.mouseY; }
float Input::MouseDeltaX() { return g_State.mouseDeltaX; }
float Input::MouseDeltaY() { return g_State.mouseDeltaY; }
float Input::ScrollX() { return g_State.scrollX; }
float Input::ScrollY() { return g_State.scrollY; }
bool Input::IsMouseButtonDown(MouseButton button) {
    return g_State.mouseDown.count(Mouse::ToSDLButton(button)) > 0;
}
bool Input::WasMouseButtonPressed(MouseButton button) {
    return g_State.mousePressed.count(Mouse::ToSDLButton(button)) > 0;
}
bool Input::WasMouseButtonReleased(MouseButton button) {
    return g_State.mouseReleased.count(Mouse::ToSDLButton(button)) > 0;
}
std::vector<Gamepad> Input::ConnectedGamepads() {
    std::vector<Gamepad> result;
    result.reserve(g_State.gamepads.size());
    for (auto& entry : g_State.gamepads) {
        result.emplace_back(entry.second, entry.first);
    }
    return result;
}
void Input::SetGamepadEnabled(bool enabled) {
    g_State.gamepadEnabled = enabled;
    if (!enabled) {
        // Drop any gamepads opened before the config was applied.
        for (auto& entry : g_State.gamepads) {
            SDL_CloseGamepad(entry.second);
        }
        g_State.gamepads.clear();
    }
}
bool Input::IsGamepadEnabled() { return g_State.gamepadEnabled; }
}

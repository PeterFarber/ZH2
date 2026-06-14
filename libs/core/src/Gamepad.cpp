#include "Hockey/Core/Gamepad.hpp"
#include <algorithm>
#include <SDL3/SDL.h>
namespace Hockey {
namespace {
SDL_GamepadButton ToSDLButton(GamepadButton button) {
    switch (button) {
        case GamepadButton::South: return SDL_GAMEPAD_BUTTON_SOUTH;
        case GamepadButton::East: return SDL_GAMEPAD_BUTTON_EAST;
        case GamepadButton::West: return SDL_GAMEPAD_BUTTON_WEST;
        case GamepadButton::North: return SDL_GAMEPAD_BUTTON_NORTH;
        case GamepadButton::Back: return SDL_GAMEPAD_BUTTON_BACK;
        case GamepadButton::Start: return SDL_GAMEPAD_BUTTON_START;
        case GamepadButton::LeftShoulder: return SDL_GAMEPAD_BUTTON_LEFT_SHOULDER;
        case GamepadButton::RightShoulder: return SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER;
        case GamepadButton::LeftStick: return SDL_GAMEPAD_BUTTON_LEFT_STICK;
        case GamepadButton::RightStick: return SDL_GAMEPAD_BUTTON_RIGHT_STICK;
    }
    return SDL_GAMEPAD_BUTTON_INVALID;
}
SDL_GamepadAxis ToSDLAxis(GamepadAxis axis) {
    switch (axis) {
        case GamepadAxis::LeftX: return SDL_GAMEPAD_AXIS_LEFTX;
        case GamepadAxis::LeftY: return SDL_GAMEPAD_AXIS_LEFTY;
        case GamepadAxis::RightX: return SDL_GAMEPAD_AXIS_RIGHTX;
        case GamepadAxis::RightY: return SDL_GAMEPAD_AXIS_RIGHTY;
        case GamepadAxis::LeftTrigger: return SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
        case GamepadAxis::RightTrigger: return SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
    }
    return SDL_GAMEPAD_AXIS_INVALID;
}
}
Gamepad::Gamepad(SDL_Gamepad* handle, int instanceId)
    : m_Handle(handle), m_InstanceId(instanceId) {}
bool Gamepad::IsConnected() const {
    return m_Handle != nullptr && SDL_GamepadConnected(m_Handle);
}
int Gamepad::InstanceId() const { return m_InstanceId; }
std::string Gamepad::Name() const {
    if (m_Handle == nullptr) {
        return {};
    }
    const char* name = SDL_GetGamepadName(m_Handle);
    return name != nullptr ? std::string(name) : std::string{};
}
bool Gamepad::IsButtonDown(GamepadButton button) const {
    return m_Handle != nullptr && SDL_GetGamepadButton(m_Handle, ToSDLButton(button));
}
float Gamepad::GetAxis(GamepadAxis axis) const {
    if (m_Handle == nullptr) {
        return 0.0f;
    }
    const Sint16 value = SDL_GetGamepadAxis(m_Handle, ToSDLAxis(axis));
    return static_cast<float>(value) / 32767.0f;
}
void Gamepad::Rumble(float lowFrequency, float highFrequency, uint32_t durationMs) const {
    if (m_Handle == nullptr) {
        return;
    }
    const float low = std::clamp(lowFrequency, 0.0f, 1.0f);
    const float high = std::clamp(highFrequency, 0.0f, 1.0f);
    SDL_RumbleGamepad(m_Handle,
                      static_cast<Uint16>(low * 0xFFFF),
                      static_cast<Uint16>(high * 0xFFFF),
                      durationMs);
}
}

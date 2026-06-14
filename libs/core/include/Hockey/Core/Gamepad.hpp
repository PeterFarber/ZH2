#pragma once
#include <cstdint>
#include <string>
struct SDL_Gamepad;
namespace Hockey {
enum class GamepadButton {
    South,
    East,
    West,
    North,
    Back,
    Start,
    LeftShoulder,
    RightShoulder,
    LeftStick,
    RightStick
};
enum class GamepadAxis {
    LeftX,
    LeftY,
    RightX,
    RightY,
    LeftTrigger,
    RightTrigger
};
// Non-owning view of an SDL gamepad. The Input system owns the underlying
// SDL handle (open/close); copying a Gamepad copies the view, not ownership.
class Gamepad {
public:
    Gamepad() = default;
    Gamepad(SDL_Gamepad* handle, int instanceId);

    bool IsConnected() const;
    int InstanceId() const;
    std::string Name() const;

    bool IsButtonDown(GamepadButton button) const;
    float GetAxis(GamepadAxis axis) const;

    void Rumble(float lowFrequency, float highFrequency, uint32_t durationMs) const;

private:
    SDL_Gamepad* m_Handle = nullptr;
    int m_InstanceId = -1;
};
}

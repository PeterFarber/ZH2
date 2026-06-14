#pragma once
namespace Hockey {
enum class KeyCode {
    Unknown,
    Escape,
    Space,
    W,
    A,
    S,
    D,
    Up,
    Down,
    Left,
    Right
};
class Keyboard {
public:
    // Maps between the engine KeyCode enum and the raw SDL scancode integer
    // stored in Event::key. Returns -1 / KeyCode::Unknown when unmapped.
    static int ToScancode(KeyCode key);
    static KeyCode FromScancode(int scancode);
};
}

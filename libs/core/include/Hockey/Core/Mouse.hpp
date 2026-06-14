#pragma once
namespace Hockey {
enum class MouseButton {
    Left,
    Right,
    Middle
};
class Mouse {
public:
    // Maps between the engine MouseButton enum and the raw SDL button index
    // stored in Event::mouseButton.
    static int ToSDLButton(MouseButton button);
    static MouseButton FromSDLButton(int button);
};
}

#include "Hockey/Core/Mouse.hpp"
#include <SDL3/SDL.h>
namespace Hockey {
int Mouse::ToSDLButton(MouseButton button) {
    switch (button) {
        case MouseButton::Left: return SDL_BUTTON_LEFT;
        case MouseButton::Right: return SDL_BUTTON_RIGHT;
        case MouseButton::Middle: return SDL_BUTTON_MIDDLE;
    }
    return SDL_BUTTON_LEFT;
}
MouseButton Mouse::FromSDLButton(int button) {
    switch (button) {
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_LEFT:
        default:
            return MouseButton::Left;
    }
}
}

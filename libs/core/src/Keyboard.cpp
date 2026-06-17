#include "Hockey/Core/Keyboard.hpp"
#include <SDL3/SDL.h>
namespace Hockey {
int Keyboard::ToScancode(KeyCode key) {
    switch (key) {
        case KeyCode::Escape: return SDL_SCANCODE_ESCAPE;
        case KeyCode::Space: return SDL_SCANCODE_SPACE;
        case KeyCode::W: return SDL_SCANCODE_W;
        case KeyCode::A: return SDL_SCANCODE_A;
        case KeyCode::S: return SDL_SCANCODE_S;
        case KeyCode::D: return SDL_SCANCODE_D;
        case KeyCode::R: return SDL_SCANCODE_R;
        case KeyCode::Up: return SDL_SCANCODE_UP;
        case KeyCode::Down: return SDL_SCANCODE_DOWN;
        case KeyCode::Left: return SDL_SCANCODE_LEFT;
        case KeyCode::Right: return SDL_SCANCODE_RIGHT;
        case KeyCode::Unknown:
        default:
            return -1;
    }
}
KeyCode Keyboard::FromScancode(int scancode) {
    switch (scancode) {
        case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
        case SDL_SCANCODE_SPACE: return KeyCode::Space;
        case SDL_SCANCODE_W: return KeyCode::W;
        case SDL_SCANCODE_A: return KeyCode::A;
        case SDL_SCANCODE_S: return KeyCode::S;
        case SDL_SCANCODE_D: return KeyCode::D;
        case SDL_SCANCODE_R: return KeyCode::R;
        case SDL_SCANCODE_UP: return KeyCode::Up;
        case SDL_SCANCODE_DOWN: return KeyCode::Down;
        case SDL_SCANCODE_LEFT: return KeyCode::Left;
        case SDL_SCANCODE_RIGHT: return KeyCode::Right;
        default: return KeyCode::Unknown;
    }
}
}

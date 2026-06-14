#include "Hockey/Core/Window.hpp"
#include "Hockey/Core/Log.hpp"
#include <SDL3/SDL.h>
namespace Hockey {
Window::~Window() {
    Destroy();
}
bool Window::Create(const WindowDesc& desc) {
    SDL_WindowFlags flags = 0;
    if (desc.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (desc.maximized) {
        flags |= SDL_WINDOW_MAXIMIZED;
    }
    if (desc.vulkan) {
        flags |= SDL_WINDOW_VULKAN;
    }
    m_Window = SDL_CreateWindow(desc.title.c_str(), static_cast<int>(desc.width), static_cast<int>(desc.height), flags);
    if (m_Window == nullptr) {
        HK_CORE_ERROR("SDL_CreateWindow failed (driver '{}'): {}",
                      SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none", SDL_GetError());
        return false;
    }
    if (desc.startCentered) {
        SDL_SetWindowPosition(m_Window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(m_Window, &width, &height);
    m_Width = static_cast<uint32_t>(width);
    m_Height = static_cast<uint32_t>(height);
    m_ShouldClose = false;
    m_Minimized = false;
    m_Focused = true;
    return true;
}
void Window::Destroy() {
    if (m_Window != nullptr) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
}
void Window::PollEvents(EventQueue& events) {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        if (m_RawEventObserver) {
            m_RawEventObserver(&sdlEvent);
        }
        Event event;
        switch (sdlEvent.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            event.type = EventType::WindowClose;
            m_ShouldClose = true;
            events.Push(event);
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            m_Width = static_cast<uint32_t>(sdlEvent.window.data1);
            m_Height = static_cast<uint32_t>(sdlEvent.window.data2);
            event.type = EventType::WindowResize;
            event.windowWidth = m_Width;
            event.windowHeight = m_Height;
            events.Push(event);
            break;
        case SDL_EVENT_WINDOW_MINIMIZED:
            m_Minimized = true;
            event.type = EventType::WindowMinimized;
            events.Push(event);
            break;
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
            m_Minimized = false;
            event.type = EventType::WindowRestored;
            events.Push(event);
            break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            m_Focused = true;
            event.type = EventType::WindowFocusGained;
            events.Push(event);
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            m_Focused = false;
            event.type = EventType::WindowFocusLost;
            events.Push(event);
            break;
        case SDL_EVENT_KEY_DOWN:
            event.type = EventType::KeyPressed;
            event.key = static_cast<int>(sdlEvent.key.scancode);
            event.keyRepeat = sdlEvent.key.repeat;
            events.Push(event);
            break;
        case SDL_EVENT_KEY_UP:
            event.type = EventType::KeyReleased;
            event.key = static_cast<int>(sdlEvent.key.scancode);
            events.Push(event);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            event.type = EventType::MouseMoved;
            event.mouseX = sdlEvent.motion.x;
            event.mouseY = sdlEvent.motion.y;
            event.mouseDeltaX = sdlEvent.motion.xrel;
            event.mouseDeltaY = sdlEvent.motion.yrel;
            events.Push(event);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            event.type = EventType::MouseButtonPressed;
            event.mouseButton = sdlEvent.button.button;
            event.mouseX = sdlEvent.button.x;
            event.mouseY = sdlEvent.button.y;
            events.Push(event);
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            event.type = EventType::MouseButtonReleased;
            event.mouseButton = sdlEvent.button.button;
            event.mouseX = sdlEvent.button.x;
            event.mouseY = sdlEvent.button.y;
            events.Push(event);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            event.type = EventType::MouseScrolled;
            event.scrollX = sdlEvent.wheel.x;
            event.scrollY = sdlEvent.wheel.y;
            events.Push(event);
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            event.type = EventType::GamepadConnected;
            event.gamepadId = static_cast<int>(sdlEvent.gdevice.which);
            events.Push(event);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            event.type = EventType::GamepadDisconnected;
            event.gamepadId = static_cast<int>(sdlEvent.gdevice.which);
            events.Push(event);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            event.type = EventType::GamepadButtonPressed;
            event.gamepadId = static_cast<int>(sdlEvent.gbutton.which);
            event.gamepadButton = sdlEvent.gbutton.button;
            events.Push(event);
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            event.type = EventType::GamepadButtonReleased;
            event.gamepadId = static_cast<int>(sdlEvent.gbutton.which);
            event.gamepadButton = sdlEvent.gbutton.button;
            events.Push(event);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            event.type = EventType::GamepadAxisMoved;
            event.gamepadId = static_cast<int>(sdlEvent.gaxis.which);
            event.gamepadAxis = sdlEvent.gaxis.axis;
            event.gamepadAxisValue = static_cast<float>(sdlEvent.gaxis.value) / 32767.0f;
            events.Push(event);
            break;
        default:
            break;
        }
    }
}
bool Window::ShouldClose() const {
    return m_ShouldClose;
}
void Window::RequestClose() {
    m_ShouldClose = true;
}
uint32_t Window::Width() const {
    return m_Width;
}
uint32_t Window::Height() const {
    return m_Height;
}
bool Window::IsMinimized() const {
    return m_Minimized;
}
bool Window::IsFocused() const {
    return m_Focused;
}
SDL_Window* Window::SDLHandle() const {
    return m_Window;
}
void Window::SetRawEventObserver(RawEventObserver observer) {
    m_RawEventObserver = std::move(observer);
}
} // namespace Hockey

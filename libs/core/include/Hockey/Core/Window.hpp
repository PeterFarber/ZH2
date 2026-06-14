#pragma once
#include "Hockey/Core/EventQueue.hpp"
#include <cstdint>
#include <functional>
#include <string>
struct SDL_Window;
namespace Hockey {
struct WindowDesc {
    std::string title = "Hockey App";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool resizable = true;
    bool maximized = false;
    bool startCentered = true;
    // When true the SDL window is created with SDL_WINDOW_VULKAN so a Vulkan
    // surface can be created from it. SDL loads the Vulkan loader on demand.
    bool vulkan = false;
};
class Window {
public:
    Window() = default;
    ~Window();

    bool Create(const WindowDesc& desc);
    void Destroy();

    void PollEvents(EventQueue& events);

    bool ShouldClose() const;
    void RequestClose();

    uint32_t Width() const;
    uint32_t Height() const;

    bool IsMinimized() const;
    bool IsFocused() const;

    SDL_Window* SDLHandle() const;

    // Registers an observer invoked with each raw SDL_Event (passed as a const
    // void* pointing at the SDL_Event) during PollEvents, before translation to
    // the engine Event type. The editor uses this to feed ImGui's SDL3 backend
    // without exposing SDL in its own headers. Pass {} to clear.
    using RawEventObserver = std::function<void(const void*)>;
    void SetRawEventObserver(RawEventObserver observer);

private:
    SDL_Window* m_Window = nullptr;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    bool m_ShouldClose = false;
    bool m_Minimized = false;
    bool m_Focused = true;
    RawEventObserver m_RawEventObserver;
};
} // namespace Hockey

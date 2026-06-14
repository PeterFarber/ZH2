#include "Hockey/Core/Timer.hpp"
namespace Hockey {
Timer::Timer() : m_Start(std::chrono::steady_clock::now()) {}
void Timer::Reset() { m_Start = std::chrono::steady_clock::now(); }
double Timer::ElapsedSeconds() const {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_Start).count();
}
double Timer::ElapsedMilliseconds() const { return ElapsedSeconds() * 1000.0; }
}

#pragma once
#include <chrono>
namespace Hockey { class Timer { public: Timer(); void Reset(); double ElapsedSeconds() const; double ElapsedMilliseconds() const; private: std::chrono::steady_clock::time_point m_Start; }; }

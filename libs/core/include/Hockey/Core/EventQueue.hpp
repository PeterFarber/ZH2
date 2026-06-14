#pragma once
#include <queue>
#include "Hockey/Core/Event.hpp"
namespace Hockey { class EventQueue { public: void Push(const Event& event); bool Poll(Event& outEvent); void Clear(); bool Empty() const; private: std::queue<Event> m_Events; }; }

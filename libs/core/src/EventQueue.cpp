#include "Hockey/Core/EventQueue.hpp"
namespace Hockey {
void EventQueue::Push(const Event& event) { m_Events.push(event); }
bool EventQueue::Poll(Event& outEvent) {
    if (m_Events.empty()) {
        return false;
    }
    outEvent = m_Events.front();
    m_Events.pop();
    return true;
}
void EventQueue::Clear() {
    std::queue<Event> empty;
    m_Events.swap(empty);
}
bool EventQueue::Empty() const { return m_Events.empty(); }
}

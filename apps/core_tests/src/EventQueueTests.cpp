#include "Test.hpp"
#include "Hockey/Core/EventQueue.hpp"
using namespace Hockey;
void RunEventQueueTests() {
    HockeyTest::BeginSuite("EventQueueTests");

    EventQueue queue;
    HK_CHECK(queue.Empty());

    Event pushed;
    pushed.type = EventType::KeyPressed;
    pushed.key = 42;
    queue.Push(pushed);
    HK_CHECK(!queue.Empty());

    Event popped;
    HK_CHECK(queue.Poll(popped));
    HK_CHECK(popped.type == EventType::KeyPressed);
    HK_CHECK_EQ(popped.key, 42);

    HK_CHECK(!queue.Poll(popped));

    queue.Push(pushed);
    queue.Push(pushed);
    queue.Clear();
    HK_CHECK(queue.Empty());
    HK_CHECK(!queue.Poll(popped));
}

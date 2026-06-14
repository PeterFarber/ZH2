#pragma once
namespace Hockey {
class SignalHandler {
public:
    static void Install();
    static bool ShutdownRequested();
    static void Reset();
};
}

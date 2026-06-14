#include "Hockey/Physics/Jolt/JoltCommon.hpp"

#include <cstdarg>
#include <cstdio>

#include "Hockey/Core/Log.hpp"

namespace Hockey::JoltDetail {

namespace {

bool s_Registered = false;

void TraceImpl(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HK_CORE_TRACE("[Jolt] {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS
bool AssertFailedImpl(const char* expression, const char* message, const char* file, JPH::uint line) {
    HK_CORE_ERROR("[Jolt] Assert failed: {}:{} ({}) {}", file, line, expression, message != nullptr ? message : "");
    return true; // trigger a breakpoint in a debugger
}
#endif

} // namespace

bool RegisterGlobals() {
    if (s_Registered) {
        return true;
    }

    JPH::RegisterDefaultAllocator();

    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    if (JPH::Factory::sInstance == nullptr) {
        JPH::Factory::sInstance = new JPH::Factory();
    }

    JPH::RegisterTypes();

    s_Registered = true;
    return true;
}

void UnregisterGlobals() {
    if (!s_Registered) {
        return;
    }

    JPH::UnregisterTypes();

    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;

    s_Registered = false;
}

bool GlobalsRegistered() {
    return s_Registered;
}

} // namespace Hockey::JoltDetail

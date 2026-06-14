#include "Hockey/Physics/Physics.hpp"

#include "Hockey/Core/Log.hpp"
#include "Hockey/Physics/Jolt/JoltCommon.hpp"

namespace Hockey {

namespace {
bool s_Initialized = false;
}

Status Physics::Init() {
    if (s_Initialized) {
        return Status::Ok();
    }

    if (!JoltDetail::RegisterGlobals()) {
        return Status::Fail("Jolt global registration failed");
    }

    s_Initialized = true;
    HK_CORE_INFO("Physics initialized (Jolt)");
    return Status::Ok();
}

void Physics::Shutdown() {
    if (!s_Initialized) {
        return;
    }

    JoltDetail::UnregisterGlobals();
    s_Initialized = false;
    HK_CORE_INFO("Physics shut down");
}

bool Physics::IsInitialized() {
    return s_Initialized;
}

} // namespace Hockey

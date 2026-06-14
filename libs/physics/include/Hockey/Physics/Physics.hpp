#pragma once

#include "Hockey/Core/Result.hpp"

namespace Hockey {

// ---------------------------------------------------------------------------
// Process-wide Jolt lifecycle. Owns the global allocator, factory and type
// registration that every PhysicsWorld depends on. This is intentionally a
// thin, header-only-friendly facade: nothing here exposes Jolt types, so the
// rest of the engine never sees <Jolt/...> headers.
//
// Rules:
//   - Init() may be called repeatedly; only the first call does real work and
//     subsequent calls are no-ops that still report success.
//   - Shutdown() is always safe (no-op when not initialised). No PhysicsWorld
//     may outlive Shutdown().
// ---------------------------------------------------------------------------
class Physics {
public:
    static Status Init();
    static void Shutdown();
    static bool IsInitialized();
};

} // namespace Hockey

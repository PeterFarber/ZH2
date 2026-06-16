#pragma once
#include <cstdlib>
#include "Hockey/Core/Log.hpp"
#if HK_CONFIG_DEBUG
#define HK_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            HK_CORE_CRITICAL("Assertion failed: " message); \
            std::abort(); \
        } \
    } while (false)
#else
#define HK_ASSERT(condition, message) \
    do { \
    } while (false)
#endif

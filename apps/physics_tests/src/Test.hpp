#pragma once
#include <cmath>
#include <cstdio>
#include <string>
namespace HockeyTest {
struct Stats {
    int passed = 0;
    int failed = 0;
};
inline Stats& GetStats() {
    static Stats stats;
    return stats;
}
inline void RecordPass() {
    ++GetStats().passed;
}
inline void RecordFail(const char* file, int line, const std::string& message) {
    ++GetStats().failed;
    std::fprintf(stderr, "[FAIL] %s:%d  %s\n", file, line, message.c_str());
}
inline void BeginSuite(const char* name) {
    std::fprintf(stderr, "[RUN ] %s\n", name);
}
} // namespace HockeyTest
#define HK_CHECK(condition)                                                                                            \
    do {                                                                                                               \
        if (condition) {                                                                                               \
            ::HockeyTest::RecordPass();                                                                                \
        } else {                                                                                                       \
            ::HockeyTest::RecordFail(__FILE__, __LINE__, "HK_CHECK(" #condition ")");                                  \
        }                                                                                                              \
    } while (false)
#define HK_CHECK_MSG(condition, message)                                                                               \
    do {                                                                                                               \
        if (condition) {                                                                                               \
            ::HockeyTest::RecordPass();                                                                                \
        } else {                                                                                                       \
            ::HockeyTest::RecordFail(__FILE__, __LINE__, (message));                                                   \
        }                                                                                                              \
    } while (false)
#define HK_CHECK_EQ(lhs, rhs)                                                                                          \
    do {                                                                                                               \
        if ((lhs) == (rhs)) {                                                                                          \
            ::HockeyTest::RecordPass();                                                                                \
        } else {                                                                                                       \
            ::HockeyTest::RecordFail(__FILE__, __LINE__, "HK_CHECK_EQ(" #lhs ", " #rhs ")");                           \
        }                                                                                                              \
    } while (false)
#define HK_CHECK_NEAR(lhs, rhs, eps)                                                                                   \
    do {                                                                                                               \
        if (std::fabs(static_cast<double>(lhs) - static_cast<double>(rhs)) <= (eps)) {                                 \
            ::HockeyTest::RecordPass();                                                                                \
        } else {                                                                                                       \
            ::HockeyTest::RecordFail(__FILE__, __LINE__, "HK_CHECK_NEAR(" #lhs ", " #rhs ")");                         \
        }                                                                                                              \
    } while (false)

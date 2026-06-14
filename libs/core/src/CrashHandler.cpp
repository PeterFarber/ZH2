#include "Hockey/Core/CrashHandler.hpp"
#include <cstdlib>
#include <exception>
#include "Hockey/Core/Log.hpp"
namespace Hockey {
namespace {
std::terminate_handler g_PreviousHandler = nullptr;

void OnTerminate() {
    if (auto logger = Log::Core()) {
        if (auto current = std::current_exception()) {
            try {
                std::rethrow_exception(current);
            } catch (const std::exception& error) {
                logger->critical("Fatal: unhandled exception: {}", error.what());
            } catch (...) {
                logger->critical("Fatal: unhandled non-standard exception");
            }
        } else {
            logger->critical("Fatal: std::terminate called");
        }
        logger->flush();
    }
    Log::Shutdown();
    if (g_PreviousHandler != nullptr) {
        g_PreviousHandler();
    }
    std::abort();
}
}
void CrashHandler::Install() { g_PreviousHandler = std::set_terminate(&OnTerminate); }
void CrashHandler::Shutdown() {
    if (g_PreviousHandler != nullptr) {
        std::set_terminate(g_PreviousHandler);
        g_PreviousHandler = nullptr;
    }
}
}

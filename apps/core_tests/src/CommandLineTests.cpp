#include "Test.hpp"
#include "Hockey/Core/CommandLine.hpp"
using namespace Hockey;
void RunCommandLineTests() {
    HockeyTest::BeginSuite("CommandLineTests");

    const char* argv[] = {
        "HockeyCoreTests", "--root", ".", "--tick-rate", "120",
        "--port", "27020", "--headless"
    };
    const int argc = 8;
    CommandLine commandLine(argc, const_cast<char**>(argv));

    HK_CHECK_EQ(commandLine.GetString("--root", ""), std::string("."));
    HK_CHECK_EQ(commandLine.GetInt("--tick-rate", 0), 120);
    HK_CHECK_EQ(commandLine.GetInt("--port", 0), 27020);
    HK_CHECK(commandLine.Has("--headless"));
    HK_CHECK(commandLine.GetBool("--headless", false));

    HK_CHECK_EQ(commandLine.GetInt("--missing", 99), 99);
    HK_CHECK_EQ(commandLine.GetString("--missing", "default"), std::string("default"));
    HK_CHECK_EQ(commandLine.GetDouble("--missing", 1.5), 1.5);
}

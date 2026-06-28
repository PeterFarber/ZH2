add_executable(HockeyCoreTests
    src/CoreTestsMain.cpp
    src/UUIDTests.cpp
    src/ConfigTests.cpp
    src/PathTests.cpp
    src/ResourceProviderTests.cpp
    src/FileSystemTests.cpp
    src/JobSystemTests.cpp
    src/FixedTickTests.cpp
    src/CommandLineTests.cpp
    src/EventQueueTests.cpp
    src/PlatformTests.cpp
    src/ThreadPoolTests.cpp
    src/TimeTests.cpp
)

target_link_libraries(HockeyCoreTests PRIVATE hockey_core)
set_target_properties(HockeyCoreTests PROPERTIES OUTPUT_NAME "HockeyCoreTests")

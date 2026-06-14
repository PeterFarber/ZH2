add_executable(HockeyCoreTests
    src/CoreTestsMain.cpp
    src/UUIDTests.cpp
    src/ConfigTests.cpp
    src/PathTests.cpp
    src/FileSystemTests.cpp
    src/JobSystemTests.cpp
    src/FixedTickTests.cpp
    src/CommandLineTests.cpp
    src/EventQueueTests.cpp
)

target_link_libraries(HockeyCoreTests PRIVATE hockey_core)
set_target_properties(HockeyCoreTests PROPERTIES OUTPUT_NAME "HockeyCoreTests")

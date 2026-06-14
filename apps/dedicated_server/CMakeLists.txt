add_executable(HockeyDedicatedServer
    src/DedicatedServerMain.cpp
    src/DedicatedServerApp.cpp
    src/DedicatedServerApp.hpp
)

target_link_libraries(HockeyDedicatedServer PRIVATE hockey_core hockey_ecs hockey_physics)
set_target_properties(HockeyDedicatedServer PROPERTIES OUTPUT_NAME "HockeyDedicatedServer")

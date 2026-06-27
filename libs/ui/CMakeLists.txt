find_package(RmlUi CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)

add_library(hockey_ui STATIC)

target_sources(hockey_ui
    PRIVATE
        src/UISettings.cpp
        src/RmlUiContext.cpp
        src/ClientFlow.cpp
        src/ClientFlowSerializer.cpp
        src/UIScreen.cpp
        src/UIScreenManager.cpp
        src/ClientFlowRunner.cpp
        src/ViewModels.cpp
        src/RmlUiRenderInterface.cpp
        src/RmlUiFileInterface.cpp
        src/RmlUiSystemInterface.cpp
        src/UIInputMapper.cpp
)

target_include_directories(hockey_ui
    PUBLIC include
    PRIVATE ${Stb_INCLUDE_DIR}
)

target_link_libraries(hockey_ui
    PUBLIC
        hockey_core
        hockey_renderer
        glm::glm
    PRIVATE
        RmlUi::RmlUi
        yaml-cpp::yaml-cpp
)

if(MSVC)
    target_compile_options(hockey_ui PRIVATE /W4)
else()
    target_compile_options(hockey_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(HOCKEY_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(hockey_ui PRIVATE /WX)
    else()
        target_compile_options(hockey_ui PRIVATE -Werror)
    endif()
endif()

cmake_minimum_required(VERSION 3.16)
project(grind VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. Pure C11 Deterministic Simulation Core + Adaptive Input Engine (Zero SDL/GL headers required)
add_library(grind_core STATIC
    src/core/grind_core.c
    src/app/grind_input.c
)
target_include_directories(grind_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(NOT WIN32)
    target_link_libraries(grind_core PUBLIC m)
endif()

# 2. Unit & Automated Game Testing Suite (CTest)
enable_testing()

if(NOT EMSCRIPTEN)
    add_executable(grind_unit_tests tests/unit/test_main.c)
    target_link_libraries(grind_unit_tests PRIVATE grind_core)
    add_test(NAME GrindUnitTests COMMAND grind_unit_tests)

    add_executable(grind_game_tests tests/game/test_game_progression.c)
    target_link_libraries(grind_game_tests PRIVATE grind_core)
    add_test(NAME GrindAutomatedGameTests COMMAND grind_game_tests)
endif()

# 3. 3D SDL2 + OpenGL Application (Linux, Windows, and Web via Emscripten)
option(GRIND_BUILD_APP "Build the 3D SDL2/OpenGL interactive application" ON)

if(GRIND_BUILD_APP)
    if(EMSCRIPTEN)
        add_executable(grind_app
            src/main.c
            src/app/grind_renderer.c
        )
        target_link_libraries(grind_app PRIVATE grind_core)
        set_target_properties(grind_app PROPERTIES SUFFIX ".html")
        target_compile_options(grind_app PRIVATE "-sUSE_SDL=2")
        target_link_options(grind_app PRIVATE
            "-sUSE_SDL=2"
            "-sUSE_WEBGL2=1"
            "-sMIN_WEBGL_VERSION=2"
            "-sMAX_WEBGL_VERSION=2"
            "-sALLOW_MEMORY_GROWTH=1"
            "--shell-file" "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        )
    else()
        find_package(SDL2 QUIET)
        find_package(OpenGL QUIET)
        if(SDL2_FOUND AND OpenGL_FOUND)
            add_executable(grind_app
                src/main.c
                src/app/grind_renderer.c
            )
            target_include_directories(grind_app PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(grind_app PRIVATE grind_core ${SDL2_LIBRARIES} OpenGL::GL)
        else()
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(SDL2 QUIET sdl2)
                pkg_check_modules(GL QUIET gl)
            endif()
            if(SDL2_FOUND AND GL_FOUND)
                add_executable(grind_app
                    src/main.c
                    src/app/grind_renderer.c
                )
                target_include_directories(grind_app PRIVATE ${SDL2_INCLUDE_DIRS} ${GL_INCLUDE_DIRS})
                target_link_libraries(grind_app PRIVATE grind_core ${SDL2_LIBRARIES} ${GL_LIBRARIES})
            else()
                message(STATUS "SDL2/OpenGL dev headers not found; building grind_core and test suites only.")
            endif()
        endif()
    endif()
endif()

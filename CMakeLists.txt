cmake_minimum_required(VERSION 3.20)
project(TinyTrails LANGUAGES C CXX)

if(WIN32 AND MINGW)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(CTest)

# Native VS Code / raylib desktop application is the first-test target.
set(GAME_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/adventure3d.cpp")

if(NOT ANDROID)
    add_subdirectory(NativeCore)
endif()

if(ANDROID)
    include(FetchContent)
    set(PLATFORM "Android" CACHE STRING "raylib target platform" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
    set(SUPPORT_MODULE_RAUDIO ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 6.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(raylib)
    add_library(main SHARED ${GAME_SOURCE})
    target_compile_features(main PRIVATE cxx_std_17)
    target_link_libraries(main PRIVATE raylib android log)
    target_link_options(main PRIVATE "-Wl,-u,ANativeActivity_onCreate")
    target_compile_definitions(main PRIVATE PLATFORM_ANDROID)
else()
    find_package(raylib CONFIG REQUIRED)
    add_executable(TinyTrails ${GAME_SOURCE})
    target_compile_features(TinyTrails PRIVATE cxx_std_17)
    target_link_libraries(TinyTrails PRIVATE raylib)
    if(MINGW)
        target_link_libraries(TinyTrails PRIVATE winmm)
    endif()

    # The old icon resource referenced the deleted lowercase assets/ folder.
    # Only compile a Windows resource when the matching icon still exists.
    if(WIN32 AND MINGW AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Assets/app_icon.ico" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_icon.rc.in")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_icon.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/windows_icon.rc"
            @ONLY
        )
        target_sources(TinyTrails PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/windows_icon.rc")
    endif()

    # Copy the game's optional Sketchfab assets beside the executable.
    if(EXISTS "${CMAKE_SOURCE_DIR}/Assets/3d")
        add_custom_target(copy_runtime_assets ALL
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/Assets/3d"
                "${CMAKE_BINARY_DIR}/Assets/3d"
            COMMENT "Copying Sketchfab models beside TinyTrails"
            VERBATIM
        )
        add_dependencies(TinyTrails copy_runtime_assets)
    endif()
endif()

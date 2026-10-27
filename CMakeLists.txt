cmake_minimum_required(VERSION 3.20)
project(TinyTrails LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(ANDROID)
    # Android builds use the official raylib source and NativeActivity toolchain.
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

    # Android's NativeActivity loads this shared object as libmain.so.
    add_library(main SHARED src/main.cpp)
    target_compile_features(main PRIVATE cxx_std_17)
    target_link_libraries(main PRIVATE raylib android log)
    target_compile_definitions(main PRIVATE PLATFORM_ANDROID)
else()
    find_package(raylib CONFIG REQUIRED)
    add_executable(TinyTrails src/main.cpp)
    target_compile_features(TinyTrails PRIVATE cxx_std_17)
    target_link_libraries(TinyTrails PRIVATE raylib)
    if(MINGW)
        target_link_libraries(TinyTrails PRIVATE winmm)
    endif()
endif()

cmake_minimum_required(VERSION 3.22)
project(WildboundStory LANGUAGES C CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "Wildbound Story is Android-only. Build through the Gradle project in android/.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

add_library(main SHARED
    src/main.cpp
)
target_compile_features(main PRIVATE cxx_std_17)
target_compile_definitions(main PRIVATE PLATFORM_ANDROID)
target_link_libraries(main PRIVATE raylib android log)
target_link_options(main PRIVATE "-Wl,-u,ANativeActivity_onCreate")

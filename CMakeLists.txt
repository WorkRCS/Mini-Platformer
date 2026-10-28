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

    # Download lightweight 1K diffuse maps from Poly Haven (CC0). The maps are
    # baked into the runtime terrain albedo, so the game has no runtime network
    # dependency. Downloads are optional: missing maps use the procedural fallback.
    set(WILDBOUND_TEXTURE_DIR "${CMAKE_BINARY_DIR}/Assets/textures")
    file(MAKE_DIRECTORY "${WILDBOUND_TEXTURE_DIR}")
    function(wildbound_download_cc0_texture asset_id)
        set(texture_file "${WILDBOUND_TEXTURE_DIR}/${asset_id}_diff_1k.jpg")
        if(NOT EXISTS "${texture_file}")
            set(texture_url "https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/${asset_id}/${asset_id}_diff_1k.jpg")
            file(DOWNLOAD "${texture_url}" "${texture_file}"
                 STATUS download_status TIMEOUT 10 TLS_VERIFY ON)
            list(GET download_status 0 download_code)
            if(NOT download_code EQUAL 0 OR NOT EXISTS "${texture_file}")
                file(REMOVE "${texture_file}")
                message(WARNING "Could not download optional Poly Haven material ${asset_id}; built-in fallback will be used.")
            else()
                message(STATUS "Installed Poly Haven CC0 terrain material: ${asset_id}")
            endif()
        endif()
    endfunction()
    wildbound_download_cc0_texture(grass_ground)
    wildbound_download_cc0_texture(sand_01)
    wildbound_download_cc0_texture(snow_01)
    wildbound_download_cc0_texture(rock_ground_02)
    wildbound_download_cc0_texture(dirt)
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

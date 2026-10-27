# Tiny Trails — Mini Platformer

A small C++17 2D platformer built with [raylib](https://www.raylib.com/). It uses the supplied animated background GIF and a transparent character sprite atlas, with a procedural fallback if those assets are not installed.

## Features

- Main menu with Play, difficulty selection, and controls
- Three short levels, platforms, coins, spikes, patrolling enemies, and a goal flag
- Score popups, heart-based lives, visible damage/respawn feedback, pause, restart, game-over, and victory screens
- Keyboard, gamepad, and on-screen touch controls
- Procedurally generated jump, coin, stomp, damage, menu, and level sound effects (no sound files needed)
- Resizable desktop window and landscape Android project

## Controls

| Action | Keyboard | Controller | Touch |
|---|---|---|---|
| Move | A/D or Left/Right | D-pad or left stick | Left/right buttons |
| Jump | Space, W, or Up | A / bottom face button | JUMP button |
| Pause | Esc or P | — | Pause button |

## Build on Windows with MSYS2 UCRT64

Open **MSYS2 UCRT64** (not PowerShell) and install the dependencies:

\`\`\`bash
pacman -S --needed git mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
\`\`\`

Download the `TinyTrails_Assets.zip` asset pack supplied with the project instructions, then extract it into the repository root. The paths should be `assets/background.gif` and `assets/character_atlas.png`. The animated background and character animation won't load until these files are present.

Clone the project and build it:

\`\`\`bash
cd /d
git clone https://github.com/WorkRCS/Mini-Platformer.git
cd Mini-Platformer
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/TinyTrails.exe
\`\`\`

If you have already cloned the repository, skip \`git clone\` and \`cd Mini-Platformer\`, then run the CMake commands from the project folder. The game is launched from the MSYS2 terminal so its raylib DLL is available on PATH.

You can also run \`bash build-msys2.sh\` from the project root after installing the dependencies.


## Fix VS Code `raylib.h` include errors

The `.vscode/c_cpp_properties.json` file configures IntelliSense for the standard MSYS2 path `C:/msys64/ucrt64/include`. Open the **repository root folder** in VS Code (not only `src/main.cpp`), then run **Developer: Reload Window** from the Command Palette.

If MSYS2 is installed somewhere other than `C:\\msys64`, update `compilerPath` and `includePath` in `.vscode/c_cpp_properties.json` to that installation. The red underline is an IntelliSense configuration warning; the CMake build uses raylib's package configuration.

## Troubleshooting gameplay feedback

- Regular platforms are solid surfaces and do not damage the player.
- Red spikes and red enemies cause damage on contact. A hit should now show the red damage banner, a visible hit ring, particles, knockback, and a short respawn pause.
- If you already cloned this project, run `git pull origin main` before rebuilding so you get the latest graphics and collision feedback.

## Android

The same game source includes touch controls and raylib gamepad input. Android Studio project files are in the \`android/\` folder. See [android/README.md](android/README.md) for the SDK/NDK requirements and APK build steps.

## Project layout

\`\`\`text
Mini-Platformer/
├── src/main.cpp
├── CMakeLists.txt
├── build-msys2.sh
└── android/
    ├── app/build.gradle
    └── app/src/main/AndroidManifest.xml
\`\`\`

The character uses a 5-column x 4-row sprite atlas (idle, walk, run, jump/landing), and the animated background uses the supplied 8-frame GIF. These same asset paths are used by the desktop build and packaged into Android when the `assets/` folder is present. Sound effects are synthesized at runtime, so no audio files are required.

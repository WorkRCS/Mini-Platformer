# Tiny Trails — Mini Platformer

A C++17 2D platformer built with [raylib](https://www.raylib.com/), using the animated waterfall background and transparent character sprite atlas in `assets/`.

## Features

- Redesigned main menu with Play, three difficulty settings (Easy / Medium / Hard), and controls
- Ten named stages: Waterfall Trail, River Crossing, Ancient Ruins, Misty Woods, Crystal Caves, Sandstone Rise, Frostline Ridge, Ember Path, Moonlit Bridges, and The Final Ascent
- Longer routes with stage-specific layouts, themes, moving platforms, thorn bushes, spikes, crystal hazards, and ember hazards
- Platforms, coins, patrolling enemies, visible hazards, and a goal flag across all ten stages
- Score popups, heart-based lives, visible damage/respawn feedback, pause, restart, game-over, and victory screens
- Difficulty-scaled movement speed, enemy speed, and hazard density; difficulty remains separate from level progression
- Keyboard, gamepad, and on-screen touch controls
- Procedurally generated jump, coin, stomp, damage, menu, and level sound effects (no sound files needed)
- Readable anti-aliased system UI font (Segoe UI on Windows, Roboto on Android, DejaVu/Liberation on Linux where installed)
- Desktop executable/window icon and Android launcher icon
- Optional image assets for cactus, pillar, large platform, and small platform
- Resizable desktop window and landscape Android project

## Controls

| Action | Keyboard | Controller | Touch |
|---|---|---|---|
| Move | A/D or Left/Right | D-pad or left stick | Left/right buttons |
| Jump | Space, W, or Up | A / bottom face button | JUMP button |
| Pause | Esc or P | — | Pause button |

## Build on Windows with MSYS2 UCRT64

Open **MSYS2 UCRT64** (not PowerShell) and install the dependencies:

```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

The background, character, and app icon files are tracked in `assets/`. The C++ code also recognises these optional transparent PNGs if you add them:
- `assets/cactus.png` — 64×96 px recommended
- `assets/pillar.png` — 64×128 px recommended
- `assets/platform_large.png` — 256×64 px recommended
- `assets/platform_small.png` — 128×32 px recommended

Those four object files are not required; clear built-in art is drawn until you add your own. If an older clone reports `Failed to open file`, run `git pull origin main` first, then re-run the CMake configure command so the assets are copied to `build/assets/`.

Clone the project and build it:

```bash
cd /d
git clone https://github.com/WorkRCS/Mini-Platformer.git
cd Mini-Platformer
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/TinyTrails.exe
```

If you have already cloned the repository, skip `git clone` and `cd Mini-Platformer`, then run the CMake commands from the project folder. The game is launched from the MSYS2 terminal so its raylib DLL is available on PATH.

You can also run `bash build-msys2.sh` from the project root after installing the dependencies.


## Fix VS Code `raylib.h` include errors

The `.vscode/c_cpp_properties.json` file configures IntelliSense for the standard MSYS2 path `C:/msys64/ucrt64/include`. Both art files (`assets/background.gif` and `assets/character_atlas.png`) are included in this repository. If your old clone shows `Failed to open file`, pull the latest changes and reconfigure CMake before launching. Open the **repository root folder** in VS Code (not only `src/main.cpp`), then run **Developer: Reload Window** from the Command Palette.

If MSYS2 is installed somewhere other than `C:\msys64`, update `compilerPath` and `includePath` in `.vscode/c_cpp_properties.json` to that installation. The red underline is an IntelliSense configuration warning; the CMake build uses raylib's package configuration.

## Troubleshooting gameplay feedback

- Regular platforms are solid surfaces and do not damage the player.
- All damaging hazards are visibly drawn and placed on a platform surface. They include thorn bushes, red spikes, crystals, and ember jets; they use the same rectangles as their collision checks.
- A hit shows a red damage banner, a visible hit ring, particles, knockback, sound, and a short respawn pause.
- If you already cloned this project, run `git pull origin main` before rebuilding so you get the ten stages, art files, and latest collision fixes.

## Android

The same game source includes touch controls and raylib gamepad input. Android Studio project files are in the `android/` folder. See [android/README.md](android/README.md) for the SDK/NDK requirements and APK build steps.

## Project layout

```text
Mini-Platformer/
├── src/main.cpp
├── CMakeLists.txt
├── build-msys2.sh
└── android/
    ├── app/build.gradle
    └── app/src/main/AndroidManifest.xml
```

The character uses one complete transparent PNG image. The game crops transparent margins at load time, preserves the image aspect ratio on every display, and uses whole-image bob/tilt/squash effects for idle, movement, jump, landing, and damage. Because this is a single pose image, it does not contain separate hand-drawn run/jump frames. The animated background uses the supplied 8-frame GIF. The same `assets/` folder is copied beside the desktop build and packaged into Android. Sound effects are synthesized at runtime, so no audio files are required.

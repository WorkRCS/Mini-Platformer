# Tiny Trails — Mini Platformer

A C++17 2D platformer built with [raylib](https://www.raylib.com/), using the animated waterfall background and one complete transparent character PNG in `assets/`.

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
- Settings screen with a local-only PIN unlock (`74330`) for invincibility/safe exploration; a Disable Cheat button immediately restores normal rules
- Bundled PNG assets for cactus, stone pillar, solid ground, medium floating platforms, and small floating platforms
- Touch-control arrows and jump icon are drawn in code; no button-image files are needed
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

The background, single-character image, and desktop/mobile app icons are tracked in `assets/`. The following PNG object assets are included and loaded automatically:
- `assets/cactus.png` — 64×96 px
- `assets/pillar.png` — 64×128 px
- `assets/platform_ground.png` — 256×64 px, for solid ground
- `assets/platform_medium.png` — 192×48 px, for medium floating platforms
- `assets/platform_small.png` — 128×40 px, for small floating platforms

Ground and floating-platform images repeat at their natural aspect ratio. Touch-control symbols are drawn using raylib primitives instead of image assets, so they remain crisp at mobile sizes. If an older clone reports a missing asset, run `git pull origin main`, then re-run CMake configure so the files are copied to `build/assets/`.

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
- A hit shows the obstacle that caused it (cactus/pillar art for those hazards), impact particles, knockback, sound, and a short respawn pause. The optional image loader preserves PNG alpha and removes only border-connected near-black matte.
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

The character uses one complete transparent PNG image. The game crops transparent margins at load time, preserves the image aspect ratio on every display, and uses whole-image bob/tilt/squash effects for idle, movement, jump, landing, and damage. Because this is a single pose image, it does not contain separate hand-drawn run/jump frames. The animated background uses the supplied 8-frame GIF. The same `assets/` folder is copied beside the desktop build and packaged into Android. The desktop executable embeds `assets/app_icon.ico`; the runtime window uses `assets/app_icon.png`, while Android uses `android/app/src/main/res/drawable/ic_launcher.xml`. Sound effects are synthesized at runtime, so no audio files are required.

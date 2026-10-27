# Wildbound: Boss Hunter — 3D Action Adventure Prototype

The game is being reworked from the old 2D platformer into an offline, third-person, open-world monster-hunting adventure in C++ and raylib. The Android package ID remains `com.workrcs.tinytrails` so this prototype can install as an update to the existing local app, while the launcher label now says **Wildbound: Boss Hunter**.

## Current playable foundation

- Third-person chase camera and 3D movement across four connected biome regions: Emerald Wilds, Golden Dunes, Frostbound Pass, and Ember Frontier
- Keyboard controls plus on-screen mobile movement joystick, Jump, Attack, and special Blink buttons
- Procedural player and monster meshes as a fallback, with a sword attack, area special ability, damage, defeat effects, particles, and rebirth/respawn feedback
- Roaming enemy creatures with health and basic chase/attack AI
- Four boss encounters with separate health pools, mission text, sequential unlocks, and a final victory state
- Offline session progression, XP, health, generated sound effects, and generated looping background music; no backend/server is required. Persistent save-to-disk is a later milestone.
- Optional loading of Kenney Nature Kit glTF/GLB trees and rocks plus optional WAV impact/UI effects, with procedural scenery and synthesized sound fallback when those files are not installed

**Important:** this is the first playable 3D foundation, not yet the final high-detail release. The current hero, creatures, and world props are procedural placeholder meshes. The next development pass is to integrate rigged character/monster GLB models and their animation clips, improve terrain/materials/lighting, add more mission types, and optimize frame time and memory for the target Android phone.

## Get optional CC0 assets automatically

From the repository root in Windows PowerShell, run this once when you have an internet connection:

```powershell
.\scripts\fetch-openworld-assets.ps1
```

The script looks up the download links from the official Kenney asset pages, downloads the Nature Kit, RPG Audio, Impact Sounds, and Interface Sounds packs, and copies selected GLB/glTF and WAV files into the game's `assets/` tree. It does not require you to browse and download each file by hand. If a pack or format changes, the script reports a warning and the game uses its built-in fallback instead. The complete source packs are cached under your Windows temp directory rather than copied wholesale into the APK.

The publisher pages identify the Kenney packs as CC0. Review the individual source page again before releasing a public/commercial build:
- [Kenney Nature Kit](https://kenney.nl/assets/nature-kit)
- [Kenney RPG Audio](https://kenney.nl/assets/rpg-audio)
- [Kenney Impact Sounds](https://kenney.nl/assets/impact-sounds)
- [Kenney Interface Sounds](https://kenney.nl/assets/interface-sounds)
- [Quaternius Bestiary — Dungeon Monsters Kit](https://quaternius.com/packs/bestiarydungeonmonsterskit.html)
- [Quaternius Universal Animation Library 2](https://quaternius.com/packs/universalanimationlibrary2.html)

Quaternius packs are not downloaded automatically by the current script. Their official page offers the free portion of the packs and marks the listed content as usable in personal and commercial projects; only add a specific download after confirming the pack's licence and format. FBX is not directly loaded by this raylib build; use GLB/glTF or export through Blender.

## Controls

| Action | Desktop | Android touch |
|---|---|---|
| Move | WASD / arrow keys | Left movement pad |
| Jump | Space | JUMP |
| Attack | J | ATTACK |
| Special Blink/AOE | K | BLINK |
| Pause / resume | Tab, P, or Esc | Android back/escape behavior or desktop pause controls |

## Android build

Open the `android/` folder in Android Studio, sync Gradle, and build the debug APK. The existing SDK/NDK/CMake versions and ARM64-only ABI configuration have been retained.

PowerShell from the repository root:

```powershell
cd android
.\gradlew.bat assembleDebug
```

APK output:
`android/app/build/outputs/apk/debug/app-debug.apk`

Install on a connected ARM64 Android phone (replace the device serial only if more than one device is connected):

```powershell
$adb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
& $adb devices
& $adb install -r ".\app\build\outputs\apk\debug\app-debug.apk"
& $adb shell monkey -p com.workrcs.tinytrails 1
```

The procedural fallback means the game can still start if you skip the asset-download step. GitHub Actions also tries to fetch these optional assets before creating its APK artifact; that fetch is allowed to fail without blocking the fallback build.

## Desktop build (MSYS2 UCRT64)

Install dependencies in MSYS2 UCRT64:

```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/TinyTrails.exe
```

## Project layout

```text
src/adventure3d.cpp               # active 3D prototype
src/main.cpp                      # preserved 2D legacy reference
scripts/fetch-openworld-assets.ps1
CMakeLists.txt
android/app/build.gradle
android/app/src/main/AndroidManifest.xml
assets/                            # optional local runtime assets
```

## Asset and licensing policy

Only download assets from their original publisher pages and verify their licences. Never bulk-mirror an asset library into a public repository or APK when the licence forbids redistribution of the source assets. The project only copies a small number of selected runtime files; keep each source pack's own licence and attribution requirements with the release notes if applicable.

# Wildbound: Boss Hunter — PC-first 3D Action Adventure

Wildbound is an offline, third-person monster-hunting adventure built with C++ and raylib. Development is **PC-first**: desktop movement, camera feel, missions, combat, enemy behavior, environment assets, and performance are the first target. Once the PC gameplay is stable, Android controls and performance will be polished as the next phase.

The Android package ID remains `com.workrcs.tinytrails` for later compatibility with the already-installed app. The launcher title is now **Wildbound: Boss Hunter**.

## Current playable foundation

- A 3D third-person world spanning four connected regions: Emerald Wilds, Golden Dunes, Frostbound Pass, and Ember Frontier
- Desktop-first controls: WASD to move relative to the camera, hold Shift to sprint, Space to jump, left click or J to attack, hold and drag the right mouse button to orbit the camera, use the mouse wheel to zoom, K to use Blink, and Tab / P / Escape to pause
- A resizable PC window, desktop HUD, mission callouts, player HP/XP, monster count, boss HP, and status messages
- Bundled-at-build CC0 GLB character models for the hero, monster, and boss, plus Kenney Nature Kit tree/rock models; optional user-installed Sketchfab models can override the character models, with procedural fallback art if downloads are unavailable
- Four sequential boss encounters with separate health pools, mission text, unlock requirements, and a final victory state
- Melee attacks, an area special ability, hit impact, particles, monster defeat effects, hurt feedback, and rebirth/respawn
- Four biome palettes with procedural trees, rocks, crystals, trails, and boss altars
- Synthesized sound effects and looping music as fallback, with optional loaded audio files
- Optional Kenney Nature Kit trees/rocks and manually installed Sketchfab model assets
- No backend/server or internet connection is required while playing

**This is the first 3D gameplay foundation, not the final high-graphics release.** Bundled CC0 character GLBs load their embedded skeletal `Idle`, `Walk`, `Run`, `Attack`, and `Hit` clips where supported; movement plays walk/run, and Shift sprints on desktop. The model loader also aligns the model base to the ground and reports animation clips in the startup log. Improved terrain and materials, richer boss patterns, save-to-disk progression, and detailed performance profiling remain further PC milestones.

## 1. Build and test the PC version first

### Windows — recommended workflow

Install [MSYS2](https://www.msys2.org/) and open **MSYS2 UCRT64**. Install the build tools:

```bash
pacman -S --needed git mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

From the repository root, fetch the free 3D character, environment, and sound assets while online (recommended for the full desktop art set):

```powershell
.\scripts\fetch-openworld-assets.ps1
```

Then run the following in the MSYS2 UCRT64 terminal:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/TinyTrails.exe
```

Run from the MSYS2 UCRT64 terminal so raylib's runtime DLLs are on PATH. Runtime assets are now copied beside the desktop executable after every build, so downloading models and running `cmake --build build` is enough; a CMake reconfigure is not required. Model-load logs identify the exact 3D files that were loaded and report when a procedural fallback is in use.

### Desktop controls

| Action | Keyboard | Mouse |
|---|---|---|
| Move relative to camera | WASD | — |
| Sprint | Hold Shift | — |
| Jump | Space | — |
| Attack | J | Left click |
| Blink special | K | — |
| Orbit camera | — | Hold right mouse button and drag |
| Zoom camera | — | Mouse wheel |
| Pause / resume | Tab, P, or Esc | — |

### PC asset setup

From Windows PowerShell, the helper script can look up official download links, fetch packs, extract them in a temporary folder, and copy only selected runtime files into `assets/`. You do not have to browse and download each chosen file manually. It is optional; the game keeps procedural environment and synthesized sound fallbacks if downloads or expected file formats are unavailable.

The desktop asset-fetch script downloads the default CC0 hero, monster, and boss models into `assets/3d/` and bundles them beside the executable, so they are available offline when the game runs. Their source and licence information is in [`assets/3d/CC0_MODEL_CREDITS.md`](assets/3d/CC0_MODEL_CREDITS.md).

The helper below can additionally install models you have downloaded from Sketchfab as overrides (it does not bypass the site login or download controls):

1. Download a free model from the source pages listed in [`assets/3d/SKETCHFAB_CREDITS.md`](assets/3d/SKETCHFAB_CREDITS.md) and extract the downloaded archive.
2. In PowerShell, pass the model file paths to the installer. Example:

```powershell
.\scripts\install-sketchfab-models.ps1 -HeroPath "C:\Users\YOUR_NAME\Downloads\hero.glb" -MonsterPath "C:\Users\YOUR_NAME\Downloads\monster.glb" -TreePath "C:\Users\YOUR_NAME\Downloads\tree.glb" -RockPath "C:\Users\YOUR_NAME\Downloads\rock.glb"
```

Use the actual paths on your PC; `.gltf` files are also supported if the adjacent textures and `.bin` files are kept together. Optional files are loaded as follows: `sketchfab_hero`, `sketchfab_monster`, `sketchfab_boss`, `sketchfab_tree`, and `sketchfab_rock` under `assets/3d/`. Review the credits file and the exact model page's current license before sharing a build. Sketchfab model downloads may require a signed-in account.

The Kenney helper below still attempts to download these packs:
- [Kenney Nature Kit](https://kenney.nl/assets/nature-kit) — CC0 3D nature models
- [Kenney RPG Audio](https://kenney.nl/assets/rpg-audio) — CC0 combat/foley sounds
- [Kenney Impact Sounds](https://kenney.nl/assets/impact-sounds) — CC0 impact sounds
- [Kenney Interface Sounds](https://kenney.nl/assets/interface-sounds) — CC0 UI sounds

Kenney says the assets on its asset pages are public-domain/CC0 and may be used in commercial projects; see its [licence FAQ](https://kenney.nl/support). Only the selected runtime assets are copied into the project. Recheck pack licences before release. Sketchfab assets are loaded as static models; GLB model loading does not itself enable run/attack animations. Those animation clips need a separate runtime animation integration step.

GitHub Actions also attempts the asset fetch before the desktop CMake configure step, then builds a Windows desktop executable. If asset downloading fails, the fallback build can still proceed.

## 2. Android comes after the PC gameplay milestone

The Android project retains the known SDK/NDK/CMake setup and ARM64-only target. Once the PC version is tested and approved, the next phase is to refine mobile touch controls, import/optimize assets, and validate performance on the target phone.

Android Studio project files are in `android/`. To build from PowerShell:

```powershell
cd android
.\gradlew.bat assembleDebug
```

APK output:
`android/app/build/outputs/apk/debug/app-debug.apk`

Install on a connected ARM64 Android phone:

```powershell
$adb = "$env:LOCALAPPDATA\Android\Sdk\platform-tools\adb.exe"
& $adb devices
& $adb install -r ".\app\build\outputs\apk\debug\app-debug.apk"
& $adb shell monkey -p com.workrcs.tinytrails 1
```

## Project layout

```text
src/adventure3d.cpp               # active 3D prototype
src/main.cpp                      # preserved 2D legacy source
scripts/fetch-openworld-assets.ps1
scripts/install-sketchfab-models.ps1
assets/3d/SKETCHFAB_CREDITS.md
CMakeLists.txt
.github/workflows/build.yml       # PC build and optional asset setup
.github/workflows/android-build.yml
android/app/build.gradle
android/app/src/main/AndroidManifest.xml
assets/                            # optional local runtime assets
```

## Asset and licensing policy

Fetch only from original publishers and verify the relevant licence before bundling. Do not bulk-mirror full asset libraries into the public repository or APK unless permitted by the licence. The current scene uses fallback meshes when imported files are absent, so the project remains buildable without downloading third-party content.

# Wildbound: Boss Hunter — Unity + C++ cross-platform game

**Unity is now the primary development and test environment.** Clone this repository and open the repository root in Unity Hub. The Unity Editor creates a starter scene on first open; press **Play** to test the prototype, then build Windows desktop or Android from the **Wildbound** menu.

The gameplay rules (movement, jumping/gravity, attack cooldowns and damage, ability damage, monster chase AI, boss unlocks, health, kills and XP) live in a portable C++17 native core under `NativeCore/`. Unity is responsible for scene and rendering, models, animation playback, input, camera, VFX, audio and platform builds; C# scripts are the Unity/native plug-in bridge. Unity cannot directly run the old raylib desktop application as a Unity scene, so the previous raylib sources remain in `src/` only as legacy reference and are no longer the default build target.

## 1. Open in Unity

1. Install **Unity 6.3 LTS** in Unity Hub (the repository pins an initial version in `ProjectSettings/ProjectVersion.txt`). Include **Windows Build Support** and **Android Build Support** with Android SDK, NDK and OpenJDK if you plan to build APKs.
2. Clone this repository.
3. In Unity Hub select **Add / Open project** and choose the cloned repository's root folder — the folder that contains `Assets`, `Packages`, and `ProjectSettings`.
4. Wait for package resolution and C# compilation. The editor setup script generates `Assets/Scenes/Wildbound.unity` and adds it to Build Settings automatically.
5. Press **Play**. Until you build the native plug-in, Play Mode shows a managed preview. For the actual C++ gameplay simulation, build the native core using **Wildbound > Build Native C++ Core (Windows x64)** or run:
   ```powershell
   .\scripts\build-native-core.ps1
   ```
   This requires CMake and a C++ compiler such as Visual Studio Build Tools. The DLL is created locally under `Assets/Plugins/x86_64/` and is intentionally not committed.

## 2. Add every 3D model from Sketchfab in Unity

Sketchfab is the **only external 3D model provider configured for this project**. No Kenney, Gobkit, itch.io or other model-provider downloads are part of the Unity workflow.

1. In Unity open **Wildbound > Sketchfab Asset Importer** and click a category search (Characters, Monsters, Environment, Animations or VFX).
2. Sign in to Sketchfab in your browser. Download a model only if that page says it is downloadable and its license permits your use. Keep the creator and license credit.
3. Unzip the download and copy the model and its textures into the appropriate folder under `Assets/Sketchfab/`. FBX uses Unity's built-in importer; GLB/glTF is handled by the pinned `com.unity.cloud.gltfast` package.
4. Create a prefab if necessary, select it in the Project window, then use **Wildbound > Sketchfab Asset Importer** to register the prefab's role and record model URL, author and license. References are stored in `Assets/Resources/WildboundAssetCatalog.asset`.
5. Assign imported animation clips to `WildboundCharacterVisual` (Idle, Walk, Run, Jump, Attack, Hit, Death), or assign an Animator Controller with parameters named `Speed`, `Grounded`, `IsRunning`, `Attack`, `Jump`, `Hit` and `Dead`.

The current starter scene generates a playable landscape and visible placeholder characters so the project is testable before you add models. Replace the player, monster, boss, tree, rock and environment prefab slots in the catalog with your Sketchfab imports. You can assign a ParticleSystem prefab to the ability VFX slot and locally licensed sound clips to the SFX/BGM slots as well. The editor does not scrape Sketchfab or bypass its login/download flow; model downloads and license selection remain under your account.

Sketchfab notes: downloading requires authentication; Creative Commons model use typically requires attribution to the author and Sketchfab. See [Sketchfab Download API](https://sketchfab.com/developers/download-api/downloading-models) and [license/attribution guidelines](https://sketchfab.com/developers/download-api/guidelines).

## 3. Build for desktop and mobile

### Windows desktop

From Unity, run **Wildbound > Build Native C++ Core (Windows x64)** first, then **Wildbound > Build > Windows 64-bit**. Output goes to `Builds/Windows/Wildbound.exe`.

### Android ARM64

Install Android Build Support through Unity Hub. Build the C++ native plug-in first using **Wildbound > Build Native C++ Core (Android ARM64)**. If the automatic NDK path is not found, run PowerShell:
```powershell
.\scripts\build-native-core.ps1 -Android -NdkPath "C:\Path\To\Unity\Editor\Data\PlaybackEngines\AndroidPlayer\NDK"
```
Then select **Wildbound > Build > Android APK (ARM64)**. The APK goes to `Builds/Android/Wildbound.apk`.

### Keyboard controls

| Action | Control |
|---|---|
| Move | W / A / S / D or arrow keys |
| Run | Hold Shift |
| Jump | Space |
| Attack | Left mouse button or J |
| Ability | K |
| Camera orbit | Hold right mouse button and drag |

The Android starter controller includes touch-zone controls; refine their UI positions for your target devices in Unity.

## 4. C++ core build and CI

From the repository root:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

GitHub Actions compiles and tests the portable C++ core on Windows and Linux. It does not run the Unity Editor or produce a Unity player in CI because a licensed Unity Editor and target modules are installed via Unity Hub locally. Native plug-in binaries are built for each target machine/platform and are excluded from Git.

## Repository layout

- `Assets/` — Unity runtime scripts, Sketchfab import folders, catalog and editor tools
- `NativeCore/` — portable C++ gameplay simulation and unit tests, no raylib dependency
- `scripts/build-native-core.ps1` — builds/copies Windows DLL or Android ARM64 SO
- `Packages/manifest.json` — Unity packages including glTFast
- `ProjectSettings/` — Unity project version and build scene settings
- `src/`, `android/`, old root assets — legacy raylib/Android project files kept for reference, not used by Unity player builds

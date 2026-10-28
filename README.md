# Wildbound: Boss Hunter — VS Code / C++ 3D game

**For the first visual test, use VS Code and the raylib C++ desktop game.** Unity project files remain as an optional scaffold, but the root CMake target and VS Code tasks now build and launch the native 3D game from `src/adventure3d.cpp`.

## 3D environment added

- A real indexed triangle terrain mesh with rolling hills across four regions, not flat ground tiles.
- A higher-detail terrain albedo baked from optional Poly Haven CC0 grass, sand, snow, rock and dirt materials, with a worn trail blended into the world texture. Missing downloads gracefully fall back to generated materials.
- Player movement, monsters, bosses, trees, rocks, crystals and altars now follow the exact rendered terrain triangles. The procedural player body, face and sword rotate with movement direction.
- Tree shapes use layered tapered crowns; rocks use a low-poly 3D mesh instead of cubes.
- Sketchfab GLB/glTF files can replace the hero, monster, boss, tree and rock fallback meshes.
- VS Code tasks can configure, build, test, run and debug the C++ app. **Visual Studio is not required.**

This is still a stylized prototype rather than a finished battle-royale production, but the ground is a real 3D mesh with more natural surface materials and actual changes in elevation. CMake tries to download the 1K CC0 terrain maps from Poly Haven once at configure time; if downloads fail, the game remains playable using its built-in fallback. Imported Sketchfab characters and props can replace the fallback meshes when you add them.

## 1. Open and run from VS Code (Windows)

1. Install **VS Code** and the **C/C++** and **CMake Tools** extensions.
2. Install **MSYS2 UCRT64**. In its UCRT64 terminal, install the C++ compiler, CMake, Ninja and raylib:
   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-raylib
   ```
3. Clone this repo and open the repository root in VS Code (File → Open Folder).
4. Press `Ctrl+Shift+P` → **Tasks: Run Task** → **Run Wildbound 3D**. This configures, builds and launches the game. You can also use **Run and Debug** and select **Wildbound 3D (raylib / MSYS2 UCRT64)**.
5. The executable is `build/TinyTrails.exe`. The task sets the MSYS2 DLL path so raylib can launch from VS Code.

If MSYS2 is installed somewhere other than `C:\msys64`, adjust the paths in `.vscode/tasks.json`, `.vscode/launch.json` and `.vscode/settings.json`.

## 2. Add real Sketchfab character and environment models

Open [Sketchfab](https://sketchfab.com/), sign in, and download the chosen model from its model page if download is enabled. Keep the model textures with the model, then copy a GLB into `Assets/3d/` using one of the names documented in [Assets/3d/README.md](Assets/3d/README.md). Run **Build Wildbound** again. The loader prefers those Sketchfab GLBs over fallback character/prop meshes.

The current raylib model loader supports GLB/glTF, not FBX. Use Sketchfab's downloadable GLB/glTF format for this native C++ path. Sketchfab downloads require account authentication and model licenses/attribution must be respected. See [Sketchfab's Download API](https://sketchfab.com/developers/download-api/downloading-models) and [model license guidelines](https://sketchfab.com/developers/download-api/guidelines).

## 3. Build, test and run manually

From the MSYS2 UCRT64 terminal at the repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build -C Release --output-on-failure
./build/TinyTrails.exe
```

The GitHub Actions workflow compiles and packages a Windows first-test build and runs the portable C++ gameplay-core tests. CI compiles the app but does not launch the graphical window, so the local VS Code run is the visual test.

## Keyboard controls

| Action | Control |
|---|---|
| Move | W / A / S / D |
| Run | Hold Shift |
| Jump | Space |
| Attack | Left click or J |
| Ability | K |
| Camera orbit | Hold right mouse button and drag |
| Camera zoom | Mouse wheel |
| Pause | Tab / P / Escape |

## Repository layout

- `src/adventure3d.cpp` — raylib 3D game and procedural terrain
- `Assets/3d/` — optional Sketchfab models and textures
- `NativeCore/` — portable C++ gameplay-core library and unit tests
- `.vscode/` — build/run/debug tasks for VS Code
- `Assets/Scripts/`, `Assets/Editor/`, `Packages/`, `ProjectSettings/` — optional Unity editor scaffold

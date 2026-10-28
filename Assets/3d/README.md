# Sketchfab-only 3D models and terrain materials

The native raylib game builds a real indexed-triangle terrain mesh, samples actors and props against the exact visible triangles, and bakes material colour into a 1536x512 terrain albedo texture.

At CMake configure time, the desktop build optionally downloads 1K diffuse maps from Poly Haven under its CC0 license:

- grass_ground — emerald grass/soil
- sand_01 — dune terrain
- snow_01 — frost terrain
- rock_ground_02 — ember terrain
- dirt — worn path

They are placed beside the executable in build/Assets/textures/, baked into the terrain texture at startup, and do not require a runtime internet connection. If a download is unavailable, the game still starts and uses its procedural material fallback. Source pages: [Grass Ground](https://polyhaven.com/a/grass_ground), [Sand 01](https://polyhaven.com/a/sand_01), [Snow 01](https://polyhaven.com/a/snow_01), [Rock Ground 02](https://polyhaven.com/a/rock_ground_02), [Dirt](https://polyhaven.com/a/dirt).

For optional imported models, use Sketchfab only. Sign in to your Sketchfab account and download models from their own model pages if downloading is enabled. Copy the model and its textures into this directory using these names:

- sketchfab_hero.glb — player character
- sketchfab_monster.glb — basic enemy
- sketchfab_boss.glb — boss character
- sketchfab_tree.glb — tree
- sketchfab_rock.glb — rock

Alternatively, create a path manifest such as sketchfab_hero_path.txt or sketchfab_tree_path.txt and put the downloaded model path inside. The C++ loader searches beside the executable and then the repository. Run the VS Code Build Wildbound task after adding/changing assets.

Sketchfab's authenticated Download API is required for model downloads. The code intentionally does not bypass the sign-in, download, or license flow. Retain any required creator/license attribution. Imported model binaries are not embedded in the source tree yet.

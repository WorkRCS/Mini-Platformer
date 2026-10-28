# Sketchfab-only 3D assets (C++ / raylib)

The game now generates a real indexed-triangle terrain mesh at startup, rather than a set of flat ground tiles. Its 1536x512 albedo texture is generated from layered noise and blended biome palettes; the central dirt trail is painted into that texture. Player, enemy and boss grounding use the same terrain-height function.

For optional imported models, use Sketchfab only. Sign in to your Sketchfab account and download models from their own model pages if downloading is enabled. Copy the model and its textures into this directory using these names:

- `sketchfab_hero.glb` — player character
- `sketchfab_monster.glb` — basic enemy
- `sketchfab_boss.glb` — boss character
- `sketchfab_tree.glb` — tree
- `sketchfab_rock.glb` — rock

Alternatively, create a path manifest such as `sketchfab_hero_path.txt` or `sketchfab_tree_path.txt` and put the downloaded model path inside. The C++ loader searches beside the executable and then the repository. Run the VS Code **Build Wildbound** task after adding/changing assets.

Sketchfab's authenticated Download API is required for model downloads. The code intentionally does not bypass the sign-in, download, or license flow. Retain any required creator/license attribution. The procedural terrain is ready to test immediately; no external model binaries are embedded in the source tree yet.

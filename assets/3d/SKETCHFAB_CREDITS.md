# Sketchfab model sources and attribution

These are suggested free-download models for Wildbound. The game only loads them after you download them from Sketchfab and install them with `scripts/install-sketchfab-models.ps1`. Sketchfab downloads may require signing in. Check the live model page and retain its exact license/author details with every release.

| Slot | Suggested model | Creator | License shown on model page | Source |
|---|---|---|---|---|
| Hero | Fantasy Warrior — Free Low-Poly 3D Model | Stan (@Stas_SayHallo) | Creative Commons Attribution | https://sketchfab.com/3d-models/fantasy-warrior-free-low-poly-3d-model-6e6239ebf12a42d4bb77dda64cffef90 |
| Monster | Armored Goblin Warrior Low Poly Game Ready | 3Dima (@Hdjusj) | Creative Commons Attribution | https://sketchfab.com/3d-models/armored-goblin-warrior-low-poly-game-ready-dc643e6067794cc988d94a72181bd48c |
| Tree | Low Poly Pine Tree | nano nova (@nanonova) | Creative Commons Attribution | https://sketchfab.com/3d-models/low-poly-pine-tree-7247cf98cbe54032928b7cd9b53af620 |
| Rock | Low-Poly Rock | Random_CGI | Creative Commons Attribution | https://sketchfab.com/3d-models/low-poly-rock-24227b933929479fb5e91ef197711c38 |

## Runtime file names

The loader looks for these files under `assets/3d/`:

- `sketchfab_hero.glb` (or `sketchfab_hero_path.txt` for a .gltf package)
- `sketchfab_monster.glb` (or `sketchfab_monster_path.txt`)
- `sketchfab_boss.glb` (optional; otherwise the procedural boss is used)
- `sketchfab_tree.glb` / `sketchfab_tree_path.txt`
- `sketchfab_rock.glb` / `sketchfab_rock_path.txt`

The game has procedural fallback models, so missing slots do not prevent it from starting. Static imported meshes do not automatically provide run/attack animations; animation integration is a separate task.

## Attribution

Credit the model authors and Sketchfab in game credits or release materials whenever you distribute a build containing these models. Follow the license displayed on the exact model page at download time. Do not redistribute any downloaded file unless its license permits that use.

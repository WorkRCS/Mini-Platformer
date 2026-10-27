# Bundled CC0 3D model sources

The desktop asset setup script downloads these self-contained GLB models into `assets/3d/` and the Windows CI workflow bundles them beside the executable. The game does not download anything at runtime.

| Game slot | Installed filename | Source model | Licence |
|---|---|---|---|
| Hero | `cc0_hero.glb` | [KnightCaptain](https://gobkit.com/freebies/human003/KnightCaptain.glb) | CC0 1.0 |
| Monster | `cc0_monster.glb` | [OrcChainBrute](https://gobkit.com/freebies/human010/OrcChainBrute.glb) | CC0 1.0 |
| Boss | `cc0_boss.glb` | [BloodironJuggernaut](https://gobkit.com/freebies/human010/BloodironJuggernaut.glb) | CC0 1.0 |

Publisher model manifest and usage notes: https://gobkit.com/api/free

Gobkit identifies these models as CC0 1.0/public domain and permits commercial or personal use without attribution. Source models face +Z; the desktop renderer applies the corresponding orientation when drawing them. These are static model loads in the current raylib implementation; the source GLBs contain animation clips, but the game does not yet play those clips.

Environment models `world_tree.glb` and `world_rock.glb` are selected from [Kenney Nature Kit](https://kenney.nl/assets/nature-kit), licensed CC0. See [Kenney's licence FAQ](https://kenney.nl/support).

Optional Sketchfab models are separate overrides installed through `scripts/install-sketchfab-models.ps1`. Their individual authorship and licence notes are in [`SKETCHFAB_CREDITS.md`](SKETCHFAB_CREDITS.md).

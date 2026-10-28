# Wildbound — Android 3D Story Game

A clean, Android-first, offline third-person 3D story-game foundation built with C++17, raylib 6 and the Android NDK. The project intentionally has no desktop-game target and no Unity scaffold.

## Art direction

Realistic, grounded materials and proportions — not cartoon or toy-like characters. Use game-ready models with PBR textures, believable lighting, and a consistent scale. Visual quality must be balanced with mobile performance.

## Asset drop-in contract

Put runtime assets in the lowercase repository folder \`assets/\`. Keep the filenames below stable; replace the files without changing their names.

| Repository path | Required content |
|---|---|
| \`assets/characters/player.glb\` | Player model, rig, and embedded animation clips |
| \`assets/characters/enemy_grunt.glb\` | Basic enemy model, rig, and embedded clips |
| \`assets/characters/boss_chapter_01.glb\` | Chapter boss model, rig, and embedded clips |
| \`assets/environment/chapter_01.glb\` | Static environment scene for the first chapter |
| \`assets/audio/chapter_01.ogg\` | Optional looping chapter music |

### Character export requirements

- Deliver **GLB/glTF**, not a renamed FBX/OBJ file. Renaming an extension does not convert a model.
- Embed the character textures in the GLB when possible.
- Keep the rig and all required clips in the same GLB so the game can load them together.
- Use these clip names (case-insensitive; names should contain the words shown):
  - Player: \`Idle\`, \`Walk\`, \`Run\`, \`Jump\`, \`Attack\`, \`Hit\`, \`Death\`
  - Grunt and boss: \`Idle\`, \`Walk\` or \`Run\`, \`Attack\`, \`Hit\`, \`Death\`
- Export with transforms applied, Y up, one game unit approximately one metre, the character rooted at its feet, and its forward direction along +Z.
- Recommended first pass for Android: player about 20k–50k triangles, enemies lower where possible, and 1K textures (2K only for assets that visibly need them). Avoid 4K/8K textures for every object.
- Avoid missing textures, separate external material folders, extremely dense meshes, and animation clips that were authored for a different skeleton.

The chapter environment should use the same scale and coordinate origin as the characters. The first playable route uses walkable ground near Y=0. The initial story positions are near the origin, a gate at Z≈8, a grunt encounter around (4, 0, 12), and a boss encounter around (0, 0, 20). These are starting coordinates and can be tuned to match the environment asset.

## Where to get assets

1. **Realistic character/environment models:** [Fab — Characters & Creatures](https://www.fab.com/category/3d-characters-creatures) and [Fab — 3D assets](https://www.fab.com/category/3d). Filter for realistic style, game-ready/rigged models and appropriate file formats. Licenses and formats vary by listing; verify each item before using it.
2. **Humanoid animation library:** [Adobe Mixamo](https://www.mixamo.com/). Adobe says Mixamo is free with an Adobe ID and its characters/animations can be used in personal and commercial projects. Mixamo downloads are commonly FBX, so the character plus clips need to be exported/packed into a compatible GLB once; do not only rename \`.fbx\` to \`.glb\`. See [Adobe's FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html).
3. **Real-world PBR textures, HDRIs and props:** [Poly Haven](https://polyhaven.com/). Its models, textures and HDRIs are CC0; attribution is not required. See its [license](https://polyhaven.com/license).
4. **Additional downloadable models:** [Sketchfab](https://sketchfab.com/). Use the downloadable filter, verify the file is glTF/GLB or can be converted, and check the exact model license and attribution terms.

Keep an \`assets/ATTRIBUTION.md\` file with creator, source URL, and license for each downloaded item, even when attribution is optional.

## Build Android APK

Install Android Studio / Android SDK, NDK \`27.0.12077973\`, CMake \`4.1.2\`, and JDK 17 or 25.

On Windows PowerShell:
\`\`\`powershell
cd android
.\\gradlew.bat assembleDebug
\`\`\`

On Linux/macOS:
\`\`\`bash
cd android
chmod +x ./gradlew
./gradlew assembleDebug
\`\`\`

APK output: \`android/app/build/outputs/apk/debug/app-debug.apk\`.

The first build needs internet to download Gradle dependencies and raylib. Once installed, game content is read from the APK; gameplay does not need an internet connection.

## Current starter scope

- Android-only build target
- Third-person follow camera with touch camera orbit
- Left-side movement joystick and right-side Jump / Attack / Interact controls
- Walk/run/jump/attack animation selection from the supplied player GLB
- One small story chapter with a gate interaction, a grunt encounter, and a boss encounter
- Realistic models are loaded from the asset contract; there is no cartoon character fallback

The model files are intentionally not committed to the repository. Add your licensed assets using the exact names above.

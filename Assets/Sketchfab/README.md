# Sketchfab-only 3D asset workflow

This Unity project uses Sketchfab as its only external provider for 3D models. No other 3D model provider is configured.

1. Open Wildbound > Sketchfab Asset Importer in Unity.
2. Open a category search. Sign in to Sketchfab, confirm the model is downloadable and read its license.
3. Download it through Sketchfab; unzip the download and copy the model plus textures into the matching folder here.
4. FBX is supported by Unity's built-in importer. GLB/glTF import is provided by the pinned com.unity.cloud.gltfast package.
5. Make a prefab from the imported model if needed. Register it with its role, model page URL, creator and license in Wildbound > Sketchfab Asset Importer.
6. For animations, assign the model's imported clips to WildboundCharacterVisual fields, or use an Animator Controller with parameters Speed, Grounded, IsRunning, Attack, Jump, Hit and Dead.

Prefab slots live in Assets/Resources/WildboundAssetCatalog.asset. The world displays simple placeholders until you assign your own Sketchfab prefabs. Keep author and license attribution, and only redistribute assets under terms that allow it.

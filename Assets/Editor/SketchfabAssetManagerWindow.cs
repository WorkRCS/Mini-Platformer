using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

public sealed class SketchfabAssetManagerWindow : EditorWindow
{
    private WildboundAssetCatalog catalog;
    private WildboundAssetRole role = WildboundAssetRole.Hero;
    private GameObject selectedPrefab;
    private string modelName = "";
    private string creator = "";
    private string modelUrl = "";
    private string license = "CC BY";
    private string licenseUrl = "https://creativecommons.org/licenses/by/4.0/";

    [MenuItem("Wildbound/Sketchfab Asset Importer")]
    public static void Open() { GetWindow<SketchfabAssetManagerWindow>("Sketchfab Assets"); }

    private void OnEnable() { LoadCatalog(); }
    private void LoadCatalog() { catalog = AssetDatabase.LoadAssetAtPath<WildboundAssetCatalog>("Assets/Resources/WildboundAssetCatalog.asset"); }

    private void OnGUI()
    {
        if (catalog == null) LoadCatalog();
        EditorGUILayout.HelpBox("Sketchfab is the only external 3D model provider configured for this project. Download a model from Sketchfab in your browser, unzip it, then copy the model and textures into Assets/Sketchfab/. This tool assigns prefabs and stores attribution.", MessageType.Info);
        if (GUILayout.Button("Create / Repair Project Folders")) WildboundProjectSetup.EnsureProject();

        EditorGUILayout.Space();
        EditorGUILayout.LabelField("Open Sketchfab search", EditorStyles.boldLabel);
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("Characters")) OpenSearch("rigged character");
        if (GUILayout.Button("Monsters")) OpenSearch("monster creature rigged");
        if (GUILayout.Button("Environment")) OpenSearch("low poly forest tree rock");
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        if (GUILayout.Button("Animations")) OpenSearch("character animation");
        if (GUILayout.Button("VFX / Creatures")) OpenSearch("magic effect creature");
        EditorGUILayout.EndHorizontal();

        EditorGUILayout.Space();
        EditorGUILayout.LabelField("Register selected Sketchfab prefab", EditorStyles.boldLabel);
        role = (WildboundAssetRole)EditorGUILayout.EnumPopup("Asset role", role);
        selectedPrefab = (GameObject)EditorGUILayout.ObjectField("Imported model/prefab", selectedPrefab, typeof(GameObject), false);
        modelName = EditorGUILayout.TextField("Model name", modelName);
        creator = EditorGUILayout.TextField("Sketchfab author", creator);
        modelUrl = EditorGUILayout.TextField("Model page URL", modelUrl);
        license = EditorGUILayout.TextField("License name", license);
        licenseUrl = EditorGUILayout.TextField("License URL", licenseUrl);

        using (new EditorGUI.DisabledScope(catalog == null || selectedPrefab == null))
        {
            if (GUILayout.Button("Register prefab and save author credits", GUILayout.Height(32))) Register();
        }

        if (catalog != null)
        {
            EditorGUILayout.Space();
            EditorGUILayout.LabelField("Registered attribution entries: " + catalog.credits.Count);
            EditorGUILayout.HelpBox("Confirm the download option and license on every Sketchfab model page. Keep creator, source URL and license attribution for redistributed builds.", MessageType.Warning);
        }
    }

    private void OpenSearch(string query)
    {
        Application.OpenURL("https://sketchfab.com/search?type=models&q=" + Uri.EscapeDataString(query) + "%20downloadable");
    }

    private void Register()
    {
        if (catalog == null || selectedPrefab == null) return;
        switch (role)
        {
            case WildboundAssetRole.Hero: catalog.heroPrefab = selectedPrefab; break;
            case WildboundAssetRole.Monster: catalog.monsterPrefab = selectedPrefab; break;
            case WildboundAssetRole.Boss: catalog.bossPrefab = selectedPrefab; break;
            case WildboundAssetRole.Tree: catalog.treePrefabs = AppendUnique(catalog.treePrefabs, selectedPrefab); break;
            case WildboundAssetRole.Rock: catalog.rockPrefabs = AppendUnique(catalog.rockPrefabs, selectedPrefab); break;
            case WildboundAssetRole.Foliage: catalog.foliagePrefabs = AppendUnique(catalog.foliagePrefabs, selectedPrefab); break;
            case WildboundAssetRole.Environment: catalog.extraEnvironmentPrefabs = AppendUnique(catalog.extraEnvironmentPrefabs, selectedPrefab); break;
            case WildboundAssetRole.Vfx:
                ParticleSystem ps = selectedPrefab.GetComponentInChildren<ParticleSystem>();
                if (ps != null) catalog.abilityVfx = ps;
                else EditorUtility.DisplayDialog("VFX prefab", "This prefab has no ParticleSystem. Add one, then register it again.", "OK");
                break;
        }

        if (!string.IsNullOrWhiteSpace(modelUrl))
        {
            bool exists = false;
            foreach (SketchfabCredit old in catalog.credits) if (old.modelUrl == modelUrl) { exists = true; break; }
            if (!exists) catalog.credits.Add(new SketchfabCredit
            {
                role = role, modelName = string.IsNullOrWhiteSpace(modelName) ? selectedPrefab.name : modelName,
                creator = creator, modelUrl = modelUrl, license = license, licenseUrl = licenseUrl
            });
        }
        EditorUtility.SetDirty(catalog);
        AssetDatabase.SaveAssets();
        EditorUtility.DisplayDialog("Sketchfab asset registered", selectedPrefab.name + " has been assigned. Inspect the WildboundAssetCatalog to review roles and attribution.", "OK");
    }

    private GameObject[] AppendUnique(GameObject[] source, GameObject item)
    {
        List<GameObject> result = source == null ? new List<GameObject>() : new List<GameObject>(source);
        if (!result.Contains(item)) result.Add(item);
        return result.ToArray();
    }
}

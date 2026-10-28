using System;
using System.Diagnostics;
using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.Build.Reporting;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

[InitializeOnLoad]
public static class WildboundProjectSetup
{
    private const string ScenePath = "Assets/Scenes/Wildbound.unity";
    private const string CatalogPath = "Assets/Resources/WildboundAssetCatalog.asset";

    static WildboundProjectSetup() { EditorApplication.delayCall += EnsureProject; }

    [MenuItem("Wildbound/Setup Project and Create Starter Scene")]
    public static void EnsureProject()
    {
        if (EditorApplication.isCompiling || EditorApplication.isUpdating) return;
        Directory.CreateDirectory("Assets/Scenes");
        Directory.CreateDirectory("Assets/Resources");
        Directory.CreateDirectory("Assets/Sketchfab/Characters");
        Directory.CreateDirectory("Assets/Sketchfab/Monsters");
        Directory.CreateDirectory("Assets/Sketchfab/Environment");
        Directory.CreateDirectory("Assets/Sketchfab/Animations");
        Directory.CreateDirectory("Assets/Sketchfab/VFX");
        Directory.CreateDirectory("Assets/Sketchfab/Attribution");
        Directory.CreateDirectory("Assets/Plugins/x86_64");
        Directory.CreateDirectory("Assets/Plugins/Android/arm64-v8a");

        if (AssetDatabase.LoadAssetAtPath<WildboundAssetCatalog>(CatalogPath) == null)
        {
            WildboundAssetCatalog catalog = ScriptableObject.CreateInstance<WildboundAssetCatalog>();
            AssetDatabase.CreateAsset(catalog, CatalogPath);
            AssetDatabase.SaveAssets();
        }

        PlayerSettings.companyName = "WorkRCS";
        PlayerSettings.productName = "Wildbound: Boss Hunter";
        PlayerSettings.defaultScreenWidth = 1280;
        PlayerSettings.defaultScreenHeight = 720;
        PlayerSettings.runInBackground = true;
        PlayerSettings.SetApplicationIdentifier(NamedBuildTarget.Android, "com.workrcs.tinytrails");
        PlayerSettings.SetScriptingBackend(NamedBuildTarget.Android, ScriptingImplementation.IL2CPP);
        PlayerSettings.Android.targetArchitectures = AndroidArchitecture.ARM64;
        PlayerSettings.Android.minSdkVersion = AndroidSdkVersions.AndroidApiLevel26;

        if (!File.Exists(ScenePath)) CreateStarterScene();
        AddSceneToBuildList();
        AssetDatabase.Refresh();
    }

    private static void CreateStarterScene()
    {
        Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
        new GameObject("Wildbound Game").AddComponent<WildboundGame>();

        GameObject cameraObject = new GameObject("Main Camera");
        cameraObject.tag = "MainCamera";
        cameraObject.transform.position = new Vector3(0, 5, -8);
        cameraObject.AddComponent<Camera>();
        cameraObject.AddComponent<AudioListener>();
        cameraObject.AddComponent<WildboundCameraRig>();

        GameObject sunObject = new GameObject("Directional Light");
        Light sun = sunObject.AddComponent<Light>();
        sun.type = LightType.Directional;
        sun.intensity = 1.15f;
        sun.shadows = LightShadows.Soft;
        sunObject.transform.rotation = Quaternion.Euler(48, -35, 0);

        EditorSceneManager.SaveScene(scene, ScenePath);
        Debug.Log("Wildbound starter scene created. Import Sketchfab models with Wildbound > Sketchfab Asset Importer.");
    }

    private static void AddSceneToBuildList()
    {
        foreach (EditorBuildSettingsScene scene in EditorBuildSettings.scenes)
            if (scene.path == ScenePath && scene.enabled) return;
        EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
    }

    [MenuItem("Wildbound/Build Native C++ Core (Windows x64)")]
    public static void BuildWindowsNativeCore()
    {
        if (Application.platform != RuntimePlatform.WindowsEditor)
        {
            EditorUtility.DisplayDialog("Windows native core", "Run scripts/build-native-core.ps1 from Windows PowerShell on this machine.", "OK");
            return;
        }
        RunPowerShellScript("scripts/build-native-core.ps1", "");
    }

    [MenuItem("Wildbound/Build Native C++ Core (Android ARM64)")]
    public static void BuildAndroidNativeCore()
    {
        string dataPath = EditorApplication.applicationContentsPath;
        string ndk = Path.GetFullPath(Path.Combine(dataPath, "PlaybackEngines", "AndroidPlayer", "NDK"));
        string args = Directory.Exists(ndk) ? "-Android -NdkPath \"" + ndk + "\"" : "-Android";
        if (Application.platform == RuntimePlatform.WindowsEditor) RunPowerShellScript("scripts/build-native-core.ps1", args);
        else EditorUtility.DisplayDialog("Android native core", "Run scripts/build-native-core.ps1 -Android -NdkPath <Unity Android NDK folder> from PowerShell.", "OK");
    }

    private static void RunPowerShellScript(string relativePath, string extraArgs)
    {
        string root = Directory.GetParent(Application.dataPath).FullName;
        string script = Path.Combine(root, relativePath);
        if (!File.Exists(script))
        {
            EditorUtility.DisplayDialog("Build native core", "Script not found: " + script, "OK");
            return;
        }
        try
        {
            ProcessStartInfo info = new ProcessStartInfo("powershell.exe")
            {
                Arguments = "-NoProfile -ExecutionPolicy Bypass -File \"" + script + "\" " + extraArgs,
                WorkingDirectory = root, UseShellExecute = false, RedirectStandardOutput = true,
                RedirectStandardError = true, CreateNoWindow = true
            };
            using (Process process = Process.Start(info))
            {
                string stdout = process.StandardOutput.ReadToEnd();
                string stderr = process.StandardError.ReadToEnd();
                process.WaitForExit();
                if (!string.IsNullOrEmpty(stdout)) UnityEngine.Debug.Log(stdout);
                if (!string.IsNullOrEmpty(stderr)) UnityEngine.Debug.LogWarning(stderr);
                if (process.ExitCode == 0)
                {
                    AssetDatabase.Refresh();
                    EditorUtility.DisplayDialog("Native core", "Native C++ plug-in built and copied into Assets/Plugins.", "OK");
                }
                else EditorUtility.DisplayDialog("Native core build failed", stderr + "\nSee Console for details.", "OK");
            }
        }
        catch (Exception ex) { EditorUtility.DisplayDialog("Native core build failed", ex.Message, "OK"); }
    }

    [MenuItem("Wildbound/Build/Windows 64-bit")]
    public static void BuildWindows() { BuildPlayer(BuildTarget.StandaloneWindows64, "Builds/Windows/Wildbound.exe"); }

    [MenuItem("Wildbound/Build/Android APK (ARM64)")]
    public static void BuildAndroid() { BuildPlayer(BuildTarget.Android, "Builds/Android/Wildbound.apk"); }

    private static void BuildPlayer(BuildTarget target, string output)
    {
        AddSceneToBuildList();
        Directory.CreateDirectory(Path.GetDirectoryName(output));
        BuildPlayerOptions options = new BuildPlayerOptions
        {
            scenes = new[] { ScenePath }, locationPathName = output,
            target = target, options = BuildOptions.None
        };
        BuildReport report = BuildPipeline.BuildPlayer(options);
        if (report.summary.result == BuildResult.Succeeded)
            EditorUtility.DisplayDialog("Wildbound build", "Build succeeded:\n" + output, "OK");
        else EditorUtility.DisplayDialog("Wildbound build", "Build failed. Check Console and Build Report.", "OK");
    }
}

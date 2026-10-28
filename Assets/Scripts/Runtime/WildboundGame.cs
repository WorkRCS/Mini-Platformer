using System;
using System.Collections.Generic;
using UnityEngine;

public sealed class WildboundGame : MonoBehaviour
{
    [Header("Assign your imported Sketchfab prefabs in Wildbound > Sketchfab Asset Importer")]
    public WildboundAssetCatalog catalog;
    public GameObject heroPrefab;
    public GameObject monsterPrefab;
    public GameObject bossPrefab;

    public Transform PlayerTransform { get { return playerView != null ? playerView.root.transform : null; } }

    private sealed class ActorView
    {
        public GameObject root;
        public WildboundCharacterVisual visual;
    }

    private ActorView playerView;
    private readonly List<ActorView> monsterViews = new List<ActorView>();
    private readonly List<ActorView> bossViews = new List<ActorView>();
    private readonly List<GameObject> environment = new List<GameObject>();

    private WildboundNative.ActorState[] monsterStates;
    private WildboundNative.ActorState[] bossStates;
    private Vector3 previewPlayerPosition = new Vector3(-6, 0, 0);
    private float previewJumpVelocity;
    private float previewHealth = 100;
    private int previewKills, previewXp;
    private bool nativeReady;
    private string nativeMessage;
    private bool jumpQueued, attackQueued, abilityQueued;
    private Vector2 touchMove;
    private AudioSource audioSource;
    private float messageClock;
    private string message = "WILDBOUND — THE HUNT BEGINS";

    private void Start()
    {
        if (catalog == null) catalog = Resources.Load<WildboundAssetCatalog>("WildboundAssetCatalog");
        if (catalog != null)
        {
            if (heroPrefab == null) heroPrefab = catalog.heroPrefab;
            if (monsterPrefab == null) monsterPrefab = catalog.monsterPrefab;
            if (bossPrefab == null) bossPrefab = catalog.bossPrefab;
        }
        nativeReady = WildboundNative.TryInitialize(out nativeMessage);
        if (nativeReady)
        {
            Debug.Log(nativeMessage);
            monsterStates = new WildboundNative.ActorState[WildboundNative.MonsterCount];
            bossStates = new WildboundNative.ActorState[WildboundNative.BossCount];
        }
        else
        {
            Debug.LogWarning(nativeMessage + " Use Wildbound > Build Native C++ Core. Until then, Play Mode uses a small managed preview.");
            CreatePreviewStates();
        }

        audioSource = GetComponent<AudioSource>();
        if (audioSource == null) audioSource = gameObject.AddComponent<AudioSource>();
        audioSource.playOnAwake = false;
        if (catalog != null && catalog.backgroundMusic != null)
        {
            audioSource.clip = catalog.backgroundMusic;
            audioSource.loop = true;
            audioSource.volume = 0.35f;
            audioSource.Play();
        }
        BuildEnvironment();
        playerView = CreateActor("Player", heroPrefab, new Vector3(-6, 0, 0), 0, new Color(0.2f, 0.55f, 1f), 1.0f);
        EnsureCameraTarget();
        if (nativeReady) SyncNativeActors();
        else SpawnPreviewActors();
    }

    private void CreatePreviewStates()
    {
        monsterStates = new WildboundNative.ActorState[8];
        bossStates = new WildboundNative.ActorState[4];
        for (int i = 0; i < monsterStates.Length; i++)
        {
            monsterStates[i].x = 4 + (i % 4) * 5;
            monsterStates[i].z = (i < 4 ? -5 : 6) + ((i % 4) - 1.5f) * 1.2f;
            monsterStates[i].health = monsterStates[i].maxHealth = 48;
            monsterStates[i].alive = 1; monsterStates[i].kind = i % 4;
        }
        float[] x = { 20, 42, 64, 86 };
        float[] z = { -12, 13, -14, 14 };
        float[] hp = { 180, 260, 360, 480 };
        for (int i = 0; i < bossStates.Length; i++)
        {
            bossStates[i].x = x[i]; bossStates[i].z = z[i];
            bossStates[i].health = bossStates[i].maxHealth = hp[i];
            bossStates[i].alive = 1; bossStates[i].kind = i;
        }
    }

    private void BuildEnvironment()
    {
        GameObject ground = GameObject.CreatePrimitive(PrimitiveType.Plane);
        ground.name = "World Ground — replace with a Sketchfab landscape prefab";
        ground.transform.localScale = new Vector3(20, 1, 12);
        ground.GetComponent<Renderer>().sharedMaterial = MakeMaterial(new Color(0.25f, 0.38f, 0.24f));
        environment.Add(ground);

        UnityEngine.Random.InitState(4107);
        for (int i = 0; i < 46; i++)
        {
            Vector3 p = new Vector3(UnityEngine.Random.Range(-43, 95), 0, UnityEngine.Random.Range(-25, 25));
            if (Mathf.Abs(p.x + 6) < 7 && Mathf.Abs(p.z) < 5) continue;
            GameObject prefab = ChoosePrefab(catalog != null ? catalog.treePrefabs : null);
            GameObject tree = prefab != null ? Instantiate(prefab, p, Quaternion.Euler(0, UnityEngine.Random.Range(0, 360), 0)) : MakeFallbackTree(p, i);
            tree.name = prefab != null ? "Sketchfab Tree" : "Placeholder Tree — assign Sketchfab prefab";
            environment.Add(tree);
        }
        for (int i = 0; i < 35; i++)
        {
            Vector3 p = new Vector3(UnityEngine.Random.Range(-43, 95), 0, UnityEngine.Random.Range(-25, 25));
            if (Mathf.Abs(p.x + 6) < 5 && Mathf.Abs(p.z) < 4) continue;
            GameObject prefab = ChoosePrefab(catalog != null ? catalog.rockPrefabs : null);
            GameObject rock = prefab != null ? Instantiate(prefab, p, Quaternion.Euler(0, UnityEngine.Random.Range(0, 360), 0)) : MakeFallbackRock(p);
            rock.name = prefab != null ? "Sketchfab Rock" : "Placeholder Rock — assign Sketchfab prefab";
            environment.Add(rock);
        }
        if (catalog != null && catalog.extraEnvironmentPrefabs != null)
        {
            for (int i = 0; i < catalog.extraEnvironmentPrefabs.Length; i++)
            {
                GameObject prefab = catalog.extraEnvironmentPrefabs[i];
                if (prefab == null) continue;
                Vector3 p = new Vector3(-30 + i * 15, 0, 8 + (i % 2) * 8);
                environment.Add(Instantiate(prefab, p, Quaternion.identity));
            }
        }
    }

    private GameObject ChoosePrefab(GameObject[] prefabs)
    {
        if (prefabs == null || prefabs.Length == 0) return null;
        for (int i = 0; i < prefabs.Length; i++) if (prefabs[i] != null) return prefabs[i];
        return null;
    }

    private GameObject MakeFallbackTree(Vector3 position, int variation)
    {
        GameObject root = new GameObject("Tree Placeholder");
        root.transform.position = position;
        GameObject trunk = GameObject.CreatePrimitive(PrimitiveType.Cylinder);
        trunk.transform.SetParent(root.transform, false);
        trunk.transform.localPosition = new Vector3(0, 1.1f, 0);
        trunk.transform.localScale = new Vector3(0.35f, 1.1f, 0.35f);
        trunk.GetComponent<Renderer>().sharedMaterial = MakeMaterial(new Color(0.28f, 0.19f, 0.11f));
        GameObject crown = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        crown.transform.SetParent(root.transform, false);
        crown.transform.localPosition = new Vector3(0, 2.5f, 0);
        crown.transform.localScale = Vector3.one * (1.6f + (variation % 3) * 0.25f);
        crown.GetComponent<Renderer>().sharedMaterial = MakeMaterial(new Color(0.19f, 0.38f, 0.18f));
        return root;
    }

    private GameObject MakeFallbackRock(Vector3 position)
    {
        GameObject rock = GameObject.CreatePrimitive(PrimitiveType.Sphere);
        rock.transform.position = position + new Vector3(0, 0.35f, 0);
        rock.transform.localScale = new Vector3(1.2f, 0.7f, 0.95f);
        rock.GetComponent<Renderer>().sharedMaterial = MakeMaterial(new Color(0.38f, 0.4f, 0.39f));
        return rock;
    }

    private Material MakeMaterial(Color color)
    {
        Shader shader = Shader.Find("Universal Render Pipeline/Lit");
        if (shader == null) shader = Shader.Find("Standard");
        Material m = new Material(shader);
        m.color = color;
        return m;
    }

    private ActorView CreateActor(string actorName, GameObject prefab, Vector3 position, float yaw, Color fallbackColor, float scale)
    {
        GameObject root = new GameObject(actorName + " Root");
        root.transform.position = position;
        root.transform.rotation = Quaternion.Euler(0, yaw, 0);
        GameObject visual;
        if (prefab != null)
        {
            visual = Instantiate(prefab, root.transform);
            visual.transform.localPosition = Vector3.zero;
            visual.transform.localRotation = Quaternion.identity;
            visual.transform.localScale *= scale;
        }
        else
        {
            visual = GameObject.CreatePrimitive(PrimitiveType.Capsule);
            visual.name = actorName + " Placeholder — assign a Sketchfab prefab";
            visual.transform.SetParent(root.transform, false);
            visual.transform.localPosition = new Vector3(0, 1, 0);
            visual.transform.localScale = new Vector3(scale, scale, scale);
            Renderer renderer = visual.GetComponent<Renderer>();
            renderer.sharedMaterial = MakeMaterial(fallbackColor);
        }
        foreach (Collider col in root.GetComponentsInChildren<Collider>()) col.enabled = false;
        WildboundCharacterVisual animation = visual.GetComponent<WildboundCharacterVisual>();
        if (animation == null) animation = visual.AddComponent<WildboundCharacterVisual>();
        return new ActorView { root = root, visual = animation };
    }

    private void SpawnPreviewActors()
    {
        for (int i = 0; i < monsterStates.Length; i++)
            monsterViews.Add(CreateActor("Monster " + (i + 1), monsterPrefab, new Vector3(monsterStates[i].x, 0, monsterStates[i].z), 180, MonsterColor(i), 0.8f));
        for (int i = 0; i < bossStates.Length; i++)
            bossViews.Add(CreateActor("Boss " + (i + 1), bossPrefab, new Vector3(bossStates[i].x, 0, bossStates[i].z), 180, BossColor(i), 1.45f));
    }

    private void EnsureCameraTarget()
    {
        WildboundCameraRig rig = FindObjectOfType<WildboundCameraRig>();
        if (rig != null && playerView != null) rig.target = playerView.root.transform;
    }

    private void Update()
    {
        if (playerView == null) return;
        ReadInput(out Vector3 move, out bool jump, out bool attack, out bool ability, out bool sprint);
        jump |= jumpQueued; attack |= attackQueued; ability |= abilityQueued;
        jumpQueued = attackQueued = abilityQueued = false;

        if (nativeReady)
        {
            WildboundNative.Update(Time.deltaTime, move, jump, attack, ability, sprint);
            SyncNativeActors();
        }
        else
        {
            UpdateManagedPreview(Time.deltaTime, move, jump, attack, ability, sprint);
            SyncPreviewActors();
        }

        if (attack && catalog != null && catalog.swordHitSfx != null) PlayOneShot(catalog.swordHitSfx);
        if (ability && catalog != null && catalog.abilityVfx != null) Instantiate(catalog.abilityVfx, playerView.root.transform.position, Quaternion.identity);
        if (messageClock > 0) messageClock -= Time.deltaTime;
    }

    private void ReadInput(out Vector3 move, out bool jump, out bool attack, out bool ability, out bool sprint)
    {
        float x = 0, z = 0;
        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) x -= 1;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) x += 1;
        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) z += 1;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) z -= 1;

        jump = Input.GetKeyDown(KeyCode.Space);
        attack = Input.GetKeyDown(KeyCode.J) || Input.GetMouseButtonDown(0);
        ability = Input.GetKeyDown(KeyCode.K);
        sprint = Input.GetKey(KeyCode.LeftShift) || Input.GetKey(KeyCode.RightShift);

        if (Application.isMobilePlatform)
        {
            touchMove = Vector2.zero;
            for (int i = 0; i < Input.touchCount; i++)
            {
                Touch t = Input.GetTouch(i);
                if (t.position.x < Screen.width * 0.42f && t.position.y < Screen.height * 0.55f)
                {
                    touchMove = new Vector2((t.position.x / Mathf.Max(1, Screen.width * 0.34f) - 0.5f) * 2,
                                            (t.position.y / Mathf.Max(1, Screen.height * 0.42f) - 0.5f) * 2);
                }
                else if (t.phase == TouchPhase.Began)
                {
                    if (t.position.x > Screen.width * 0.82f && t.position.y < Screen.height * 0.34f) attackQueued = true;
                    else if (t.position.x > Screen.width * 0.62f && t.position.x < Screen.width * 0.82f && t.position.y < Screen.height * 0.24f) jumpQueued = true;
                    else if (t.position.x > Screen.width * 0.64f && t.position.x < Screen.width * 0.84f && t.position.y < Screen.height * 0.47f) abilityQueued = true;
                }
            }
            x += touchMove.x; z += touchMove.y;
            jump |= jumpQueued; attack |= attackQueued; ability |= abilityQueued;
        }

        Camera cam = Camera.main;
        Vector3 forward = cam != null ? cam.transform.forward : Vector3.forward;
        Vector3 right = cam != null ? cam.transform.right : Vector3.right;
        forward.y = 0; right.y = 0; forward.Normalize(); right.Normalize();
        move = (forward * z + right * x);
        if (move.sqrMagnitude > 1) move.Normalize();
    }

    private void UpdateManagedPreview(float dt, Vector3 move, bool jump, bool attack, bool ability, bool sprint)
    {
        dt = Mathf.Min(dt, 0.05f);
        previewPlayerPosition += move * (sprint ? 10 : 6.6f) * dt;
        previewJumpVelocity -= 22 * dt;
        if (jump && previewPlayerPosition.y <= 0.001f) previewJumpVelocity = 8.2f;
        previewPlayerPosition.y += previewJumpVelocity * dt;
        if (previewPlayerPosition.y <= 0) { previewPlayerPosition.y = 0; previewJumpVelocity = 0; }
        previewPlayerPosition.x = Mathf.Clamp(previewPlayerPosition.x, -48, 105);
        previewPlayerPosition.z = Mathf.Clamp(previewPlayerPosition.z, -27, 27);

        if (attack || ability)
        {
            for (int i = 0; i < monsterStates.Length; i++)
            {
                if (monsterStates[i].alive == 0) continue;
                Vector3 p = new Vector3(monsterStates[i].x, 0, monsterStates[i].z);
                float range = ability ? 5.2f : 3.1f;
                if (Vector3.Distance(previewPlayerPosition, p) <= range)
                {
                    monsterStates[i].health -= ability ? 65 : 34;
                    if (monsterStates[i].health <= 0)
                    {
                        monsterStates[i].health = 0; monsterStates[i].alive = 0;
                        previewKills++; previewXp += 25;
                    }
                }
            }
        }

        for (int i = 0; i < monsterStates.Length; i++)
        {
            if (monsterStates[i].alive == 0) continue;
            Vector3 enemy = new Vector3(monsterStates[i].x, 0, monsterStates[i].z);
            Vector3 delta = previewPlayerPosition - enemy;
            if (delta.magnitude < 22 && delta.magnitude > 1.8f)
            {
                delta.y = 0;
                enemy += delta.normalized * 2.0f * dt;
                monsterStates[i].x = enemy.x; monsterStates[i].z = enemy.z;
            }
            monsterStates[i].animationState = delta.magnitude > 1.8f ? 1 : 4;
        }
        foreach (ActorView view in monsterViews) { if (view != null) { } }
    }

    private void SyncNativeActors()
    {
        WildboundNative.ActorState p = WildboundNative.GetPlayer();
        ApplyState(playerView, p, 1);
        int mc = WildboundNative.MonsterCount;
        EnsureActorViewCount(monsterViews, mc, monsterPrefab, "Monster", 0.8f);
        for (int i = 0; i < mc; i++) ApplyState(monsterViews[i], WildboundNative.GetMonster(i), 0.8f);
        int bc = WildboundNative.BossCount;
        EnsureActorViewCount(bossViews, bc, bossPrefab, "Boss", 1.45f);
        for (int i = 0; i < bc; i++) ApplyState(bossViews[i], WildboundNative.GetBoss(i), 1.45f);
    }

    private void EnsureActorViewCount(List<ActorView> views, int count, GameObject prefab, string label, float scale)
    {
        while (views.Count < count)
            views.Add(CreateActor(label + " " + (views.Count + 1), prefab, Vector3.zero, 0, label == "Boss" ? BossColor(views.Count) : MonsterColor(views.Count), scale));
    }

    private void SyncPreviewActors()
    {
        if (playerView == null) return;
        playerView.root.transform.position = previewPlayerPosition;
        Vector3 movement = Vector3.zero;
        if (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow)) movement.x -= 1;
        if (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow)) movement.x += 1;
        if (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow)) movement.z -= 1;
        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow)) movement.z += 1;
        int state = previewPlayerPosition.y > 0.05f ? 3 : (movement.sqrMagnitude > 0.05f ? (Input.GetKey(KeyCode.LeftShift) ? 2 : 1) : 0);
        playerView.visual.SetAnimation(state, movement.magnitude);
        for (int i = 0; i < monsterStates.Length && i < monsterViews.Count; i++) ApplyState(monsterViews[i], monsterStates[i], 0.8f);
        for (int i = 0; i < bossStates.Length && i < bossViews.Count; i++) ApplyState(bossViews[i], bossStates[i], 1.45f);
    }

    private void ApplyState(ActorView view, WildboundNative.ActorState state, float scale)
    {
        if (view == null || view.root == null) return;
        view.root.SetActive(state.alive != 0);
        if (state.alive == 0) return;
        view.root.transform.position = new Vector3(state.x, state.y, state.z);
        view.root.transform.rotation = Quaternion.Euler(0, state.yawDegrees, 0);
        if (view.visual != null) view.visual.SetAnimation(state.animationState, state.animationState == 2 ? 1 : state.animationState == 1 ? 0.5f : 0);
    }

    private GameObject MakePrimitiveActor(PrimitiveType type, Vector3 p, Color color)
    {
        GameObject go = GameObject.CreatePrimitive(type);
        go.transform.position = p;
        go.GetComponent<Renderer>().sharedMaterial = MakeMaterial(color);
        Collider col = go.GetComponent<Collider>();
        if (col != null) col.enabled = false;
        return go;
    }

    private Color MonsterColor(int i)
    {
        Color[] colors = { new Color(.28f,.75f,.3f), new Color(.25f,.55f,.9f), new Color(.7f,.35f,.8f), new Color(.9f,.35f,.2f) };
        return colors[i % colors.Length];
    }
    private Color BossColor(int i)
    {
        Color[] colors = { new Color(.8f,.3f,.2f), new Color(.3f,.6f,.9f), new Color(.9f,.25f,.1f), new Color(.7f,.3f,.9f) };
        return colors[i % colors.Length];
    }

    private void PlayOneShot(AudioClip clip)
    {
        if (audioSource != null && clip != null) audioSource.PlayOneShot(clip);
    }

    private void OnGUI()
    {
        GUI.Box(new Rect(12, 12, 330, 86), "WILDBOUND  |  BOSS HUNTER");
        float hp = nativeReady ? WildboundNative.GetPlayer().health : previewHealth;
        float maxHp = nativeReady ? WildboundNative.GetPlayer().maxHealth : 100;
        float ratio = maxHp > 0 ? Mathf.Clamp01(hp / maxHp) : 0;
        GUI.Box(new Rect(25, 43, 240, 20), "");
        Color old = GUI.color;
        GUI.color = ratio > .3f ? new Color(.2f,.85f,.35f) : Color.red;
        GUI.Box(new Rect(28, 46, 234 * ratio, 14), "");
        GUI.color = old;
        int kills = nativeReady ? WildboundNative.wb_get_kills() : previewKills;
        int xp = nativeReady ? WildboundNative.wb_get_xp() : previewXp;
        int bosses = nativeReady ? WildboundNative.wb_get_defeated_bosses() : 0;
        GUI.Label(new Rect(25, 68, 310, 24), "HP " + Mathf.RoundToInt(hp) + "   Kills " + kills + "   XP " + xp + "   Bosses " + bosses + "/4");
        if (messageClock > 0) GUI.Box(new Rect(Screen.width * .30f, 15, Screen.width * .4f, 32), message);
        if (!nativeReady) GUI.Box(new Rect(12, 105, 400, 46), "PREVIEW MODE — build NativeCore to run C++ gameplay logic.");
        if (Application.isMobilePlatform)
        {
            GUI.Box(new Rect(16, Screen.height - 160, 155, 135), "TOUCH MOVE");
            GUI.Box(new Rect(Screen.width - 218, Screen.height - 160, 200, 135), "ATTACK    J");
            GUI.Box(new Rect(Screen.width - 218, Screen.height - 108, 96, 42), "JUMP");
            GUI.Box(new Rect(Screen.width - 116, Screen.height - 108, 98, 42), "BLINK");
        }
    }
}


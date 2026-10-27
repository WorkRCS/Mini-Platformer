#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {
constexpr int DESIGN_W = 960;
constexpr int DESIGN_H = 540;
constexpr float WORLD_MIN_X = -42.0f;
constexpr float WORLD_MAX_X = 242.0f;
constexpr float WORLD_HALF_Z = 60.0f;
constexpr int BOSS_COUNT = 4;

const Color C_INK = { 12, 18, 34, 255 };
const Color C_PANEL = { 17, 27, 47, 230 };
const Color C_MINT = { 110, 255, 204, 255 };
const Color C_GOLD = { 255, 203, 85, 255 };
const Color C_RED = { 255, 90, 101, 255 };
const Color C_VIOLET = { 184, 130, 255, 255 };
const Color C_CYAN = { 84, 221, 255, 255 };

enum class Screen { Menu, Playing, Paused, Rebirth, Victory };
enum class Biome { Emerald, Dune, Frost, Ember };

struct Particle {
    Vector3 position;
    Vector3 velocity;
    float life;
    float maxLife;
    float size;
    Color color;
};

struct Prop {
    Vector3 position;
    float scale;
    int kind; // 0 tree, 1 rock, 2 crystal, 3 dead tree, 4 bush
    int variant;
};

struct Monster {
    Vector3 position;
    Vector3 home;
    float health = 48.0f;
    float maxHealth = 48.0f;
    float speed = 2.1f;
    float attackClock = 0.0f;
    float phase = 0.0f;
    bool alive = true;
    int kind = 0;
};

struct Boss {
    Vector3 position;
    const char* name;
    const char* mission;
    float health;
    float maxHealth;
    float speed;
    float attackClock;
    float phase;
    bool alive;
};

struct Player {
    Vector3 position = { -16.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    Vector3 facing = { 0.0f, 0.0f, -1.0f };
    float health = 100.0f;
    float maxHealth = 100.0f;
    float attackClock = 0.0f;
    float attackCooldown = 0.0f;
    float abilityClock = 0.0f;
    float abilityCooldown = 0.0f;
    float invulnerable = 0.0f;
    float hurtFlash = 0.0f;
    float rebirthClock = 0.0f;
    float walkClock = 0.0f;
    bool grounded = true;
    bool attackHit = false;
    int kills = 0;
    int xp = 0;
    int defeatedBosses = 0;
};

Screen screen = Screen::Menu;
Player player;
Camera3D camera = {};
std::vector<Prop> props;
std::vector<Monster> monsters;
std::vector<Particle> particles;
Boss bosses[BOSS_COUNT] = {
    { { 30.0f, 0.0f, -13.0f }, "STONEFANG", "Hunt three shades, then defeat Stonefang", 180, 180, 2.2f, 1.2f, 0.0f, true },
    { { 88.0f, 0.0f, 14.0f }, "FROSTMAW", "Travel to the frozen pass and defeat Frostmaw", 260, 260, 2.45f, 1.8f, 0.0f, true },
    { { 151.0f, 0.0f, -17.0f }, "ASH TITAN", "Cross the ember fields and defeat the Ash Titan", 360, 360, 2.6f, 2.0f, 0.0f, true },
    { { 215.0f, 0.0f, 13.0f }, "NIGHT WARDEN", "Reach the final sanctuary and defeat the Night Warden", 480, 480, 2.8f, 2.0f, 0.0f, true }
};
int currentBoss = 0;
float worldClock = 0.0f;
float missionToastClock = 5.0f;
std::string missionToast = "EXPLORE THE WILDS";
float screenShake = 0.0f;
float attackSlashClock = 0.0f;
float abilityGlowClock = 0.0f;
float menuPulse = 0.0f;
bool audioReady = false;
bool musicEnabled = true;
Sound sfxHit = {};
Sound sfxMonsterDown = {};
Sound sfxPlayerHurt = {};
Sound sfxAbility = {};
Sound sfxBossRoar = {};
Sound sfxRebirth = {};
Sound sfxMenu = {};
Sound sfxMusic = {};
Model natureTree = {};
Model natureRock = {};
bool treeModelReady = false;
bool rockModelReady = false;

Vector2 newTouchPresses[10] = {};
int newTouchPressCount = 0;
int previousTouchIds[10] = {};
int previousTouchCount = 0;

Vector3 V3(float x, float y, float z) { return { x, y, z }; }
float DistXZ(Vector3 a, Vector3 b) {
    const float dx = a.x - b.x, dz = a.z - b.z;
    return std::sqrt(dx*dx + dz*dz);
}
Vector3 NormalizeXZ(Vector3 v) {
    const float len = std::sqrt(v.x*v.x + v.z*v.z);
    if (len < 0.001f) return V3(0.0f, 0.0f, -1.0f);
    return V3(v.x/len, 0.0f, v.z/len);
}
float ClampF(float v, float lo, float hi) { return std::max(lo, std::min(hi, v)); }
Color WithAlpha(Color c, unsigned char alpha) { c.a = alpha; return c; }

Biome GetBiome(float x) {
    if (x < 48.0f) return Biome::Emerald;
    if (x < 112.0f) return Biome::Dune;
    if (x < 174.0f) return Biome::Frost;
    return Biome::Ember;
}
const char* BiomeName(Biome b) {
    switch (b) {
        case Biome::Emerald: return "EMERALD WILDS";
        case Biome::Dune: return "GOLDEN DUNES";
        case Biome::Frost: return "FROSTBOUND PASS";
        case Biome::Ember: return "EMBER FRONTIER";
    }
    return "UNKNOWN WILDS";
}
Color GroundColor(Biome b, int variation) {
    const int n = variation % 3;
    switch (b) {
        case Biome::Emerald:
            return n == 0 ? Color{ 60, 116, 69, 255 } : (n == 1 ? Color{ 66, 124, 73, 255 } : Color{ 57, 108, 65, 255 });
        case Biome::Dune:
            return n == 0 ? Color{ 177, 142, 78, 255 } : (n == 1 ? Color{ 189, 153, 89, 255 } : Color{ 168, 132, 69, 255 });
        case Biome::Frost:
            return n == 0 ? Color{ 137, 173, 184, 255 } : (n == 1 ? Color{ 151, 189, 197, 255 } : Color{ 126, 162, 176, 255 });
        case Biome::Ember:
            return n == 0 ? Color{ 89, 63, 60, 255 } : (n == 1 ? Color{ 105, 70, 61, 255 } : Color{ 79, 54, 53, 255 });
    }
    return GREEN;
}
Color MonsterColor(int kind) {
    switch (kind % 4) {
        case 0: return { 126, 225, 124, 255 };
        case 1: return { 117, 199, 255, 255 };
        case 2: return { 202, 135, 255, 255 };
        default: return { 255, 133, 94, 255 };
    }
}
Color BossColor(int index) {
    const Color colors[BOSS_COUNT] = {
        { 208, 115, 78, 255 }, { 93, 201, 255, 255 },
        { 255, 104, 69, 255 }, { 188, 112, 255, 255 }
    };
    return colors[index % BOSS_COUNT];
}

void SpawnBurst(Vector3 where, Color c, int amount, float force) {
    for (int i = 0; i < amount; ++i) {
        if (particles.size() >= 240) particles.erase(particles.begin(), particles.begin() + 32);
        const float a = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float up = static_cast<float>(GetRandomValue(-35, 100)) / 100.0f;
        const float speed = force * static_cast<float>(GetRandomValue(45, 120)) / 100.0f;
        particles.push_back({ where,
            V3(std::cos(a)*speed, up*speed, std::sin(a)*speed),
            0.35f + static_cast<float>(GetRandomValue(0, 65))/100.0f,
            1.0f,
            0.055f + static_cast<float>(GetRandomValue(0, 65))/1000.0f,
            c });
    }
}

Sound MakeTone(float frequency, float duration, float volume, bool noisy = false) {
    const unsigned int sampleRate = 22050;
    const unsigned int frames = static_cast<unsigned int>(sampleRate * duration);
    short* samples = static_cast<short*>(MemAlloc(frames * sizeof(short)));
    if (samples == nullptr) return Sound{};
    for (unsigned int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float progress = t / duration;
        const float envelope = std::min(1.0f, t / 0.012f) * std::max(0.0f, (duration - t) / 0.045f);
        float signal = std::sin(2.0f * PI * frequency * t * (1.0f - 0.28f*progress));
        signal += 0.24f * std::sin(2.0f * PI * frequency * 1.97f * t);
        if (noisy) {
            const unsigned int seed = i * 1103515245u + 12345u;
            const float noise = static_cast<float>((seed >> 16) & 0x7fff) / 16383.5f - 1.0f;
            signal = signal*0.6f + noise*0.4f;
        }
        const float value = ClampF(signal * envelope * volume, -1.0f, 1.0f);
        samples[i] = static_cast<short>(value * 32767.0f);
    }
    Wave wave = { frames, sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}
void LoadRealSound(Sound& target, const char* path) {
    if (!FileExists(path)) return;
    Sound replacement = LoadSound(path);
    if (replacement.frameCount == 0) return;
    if (target.frameCount > 0) UnloadSound(target);
    target = replacement;
}

Sound MakeMusic() {
    const unsigned int sampleRate = 22050;
    const float duration = 8.0f;
    const unsigned int frames = static_cast<unsigned int>(sampleRate * duration);
    short* samples = static_cast<short*>(MemAlloc(frames * sizeof(short)));
    if (samples == nullptr) return Sound{};
    const float melody[16] = { 392.0f, 493.88f, 587.33f, 493.88f, 440.0f, 523.25f, 659.25f, 523.25f,
                               392.0f, 493.88f, 587.33f, 783.99f, 659.25f, 587.33f, 493.88f, 440.0f };
    const float bass[16] = { 98.0f, 98.0f, 123.47f, 123.47f, 110.0f, 110.0f, 130.81f, 130.81f,
                             98.0f, 98.0f, 123.47f, 123.47f, 82.41f, 82.41f, 110.0f, 110.0f };
    for (unsigned int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const unsigned int note = std::min(15u, static_cast<unsigned int>(t / 0.5f));
        const float within = std::fmod(t, 0.5f);
        const float env = std::min(1.0f, within / 0.02f) * std::min(1.0f, (0.5f-within) / 0.05f);
        const float loopEnv = std::min(1.0f, t / 0.08f) * std::min(1.0f, (duration-t) / 0.12f);
        const float sig = (0.45f*std::sin(2.0f*PI*melody[note]*t)*env
                         + 0.2f*std::sin(2.0f*PI*bass[note]*t)) * loopEnv * 0.45f;
        samples[i] = static_cast<short>(ClampF(sig, -1.0f, 1.0f) * 32767.0f);
    }
    Wave wave = { frames, sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}
void PlaySfx(Sound sound) {
    if (audioReady && sound.frameCount > 0) PlaySound(sound);
}
void InitAudio() {
    InitAudioDevice();
    audioReady = IsAudioDeviceReady();
    if (!audioReady) return;
    SetMasterVolume(0.78f);
    sfxHit = MakeTone(190.0f, 0.16f, 0.5f, true);
    sfxMonsterDown = MakeTone(85.0f, 0.42f, 0.55f, true);
    sfxPlayerHurt = MakeTone(125.0f, 0.25f, 0.48f, true);
    sfxAbility = MakeTone(720.0f, 0.38f, 0.5f);
    sfxBossRoar = MakeTone(65.0f, 0.65f, 0.58f, true);
    sfxRebirth = MakeTone(520.0f, 0.55f, 0.42f);
    sfxMenu = MakeTone(430.0f, 0.08f, 0.23f);
    sfxMusic = MakeMusic();
}
void UpdateMusic() {
    if (!audioReady || sfxMusic.frameCount == 0) return;
    SetSoundVolume(sfxMusic, 0.14f);
    if (!musicEnabled) {
        if (IsSoundPlaying(sfxMusic)) StopSound(sfxMusic);
        return;
    }
    if (screen != Screen::Paused && !IsSoundPlaying(sfxMusic)) PlaySound(sfxMusic);
    if (screen == Screen::Paused && IsSoundPlaying(sfxMusic)) StopSound(sfxMusic);
}
void CloseAudio() {
    if (!audioReady) return;
    Sound sounds[] = { sfxHit, sfxMonsterDown, sfxPlayerHurt, sfxAbility, sfxBossRoar, sfxRebirth, sfxMenu, sfxMusic };
    for (Sound s : sounds) if (s.frameCount > 0) UnloadSound(s);
    CloseAudioDevice();
    audioReady = false;
}

void CaptureNewTouchPresses() {
    newTouchPressCount = 0;
    const int count = std::min(GetTouchPointCount(), 10);
    int ids[10] = {};
    for (int i = 0; i < count; ++i) {
        ids[i] = GetTouchPointId(i);
        bool wasDown = false;
        for (int j = 0; j < previousTouchCount; ++j)
            if (previousTouchIds[j] == ids[i]) { wasDown = true; break; }
        if (!wasDown && newTouchPressCount < 10)
            newTouchPresses[newTouchPressCount++] = GetTouchPosition(i);
    }
    previousTouchCount = count;
    for (int i = 0; i < count; ++i) previousTouchIds[i] = ids[i];
}
bool Pressed(Rectangle r) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r)) return true;
    for (int i = 0; i < newTouchPressCount; ++i)
        if (CheckCollisionPointRec(newTouchPresses[i], r)) return true;
    return false;
}
bool Held(Rectangle r) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), r)) return true;
    const int count = GetTouchPointCount();
    for (int i = 0; i < count; ++i)
        if (CheckCollisionPointRec(GetTouchPosition(i), r)) return true;
    return false;
}
Rectangle ButtonRect(float x, float y, float w, float h) { return { x, y, w, h }; }

void MakeWorld() {
    props.clear();
    monsters.clear();
    particles.clear();
    SetRandomSeed(481516);
    for (int i = 0; i < 155; ++i) {
        const float x = static_cast<float>(GetRandomValue(-35, 236));
        const float z = static_cast<float>(GetRandomValue(-55, 55));
        if (std::fabs(z) < 5.5f && (x > -25 && x < 225)) continue; // leave a readable travel corridor
        const float scale = static_cast<float>(GetRandomValue(70, 155)) / 100.0f;
        const int kindRoll = GetRandomValue(0, 99);
        int kind = kindRoll < 50 ? 0 : (kindRoll < 76 ? 1 : (kindRoll < 88 ? 2 : 4));
        if (x > 48 && x < 112 && kind == 0) kind = 1;
        if (x > 112 && x < 174 && kind == 0) kind = 3;
        props.push_back({ V3(x, 0.0f, z), scale, kind, GetRandomValue(0, 3) });
    }
    for (int i = 0; i < 13; ++i) {
        const float x = static_cast<float>(GetRandomValue(-8, 228));
        const float z = static_cast<float>(GetRandomValue(-35, 35));
        monsters.push_back({ V3(x, 0.0f, z), V3(x, 0.0f, z),
                             48.0f, 48.0f, 1.7f + GetRandomValue(0, 12)/10.0f,
                             static_cast<float>(GetRandomValue(3, 7))/2.0f, 0.0f, true, i%4 });
    }
    for (int i=0; i<BOSS_COUNT; ++i) {
        bosses[i].health = bosses[i].maxHealth;
        bosses[i].alive = true;
        bosses[i].attackClock = 1.6f + i*0.35f;
        bosses[i].phase = 0.0f;
    }
    player = Player{};
    player.position = V3(-16.0f, 0.0f, 0.0f);
    currentBoss = 0;
    missionToast = "EXPLORE THE WILDS";
    missionToastClock = 4.5f;
    worldClock = 0.0f;
}

Model LoadModelFromManifest(const char* manifestPath) {
    if (!FileExists(manifestPath)) return Model{};
    char* raw = LoadFileText(manifestPath);
    if (raw == nullptr) return Model{};
    std::string path(raw);
    UnloadFileText(raw);
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' ' || path.back() == '\t')) path.pop_back();
    while (!path.empty() && (path.front() == ' ' || path.front() == '\t')) path.erase(path.begin());
    if (path.empty() || !FileExists(path.c_str())) return Model{};
    return LoadModel(path.c_str());
}
void LoadOptionalNatureModels() {
    // Models from the CC0 Kenney Nature Kit are optional; procedural scenery is the fallback.
    if (FileExists("assets/3d/world_tree.glb")) natureTree = LoadModel("assets/3d/world_tree.glb");
    else if (FileExists("assets/3d/world_tree.gltf")) natureTree = LoadModel("assets/3d/world_tree.gltf");
    else natureTree = LoadModelFromManifest("assets/3d/world_tree_path.txt");
    treeModelReady = natureTree.meshCount > 0;

    if (FileExists("assets/3d/world_rock.glb")) natureRock = LoadModel("assets/3d/world_rock.glb");
    else if (FileExists("assets/3d/world_rock.gltf")) natureRock = LoadModel("assets/3d/world_rock.gltf");
    else natureRock = LoadModelFromManifest("assets/3d/world_rock_path.txt");
    rockModelReady = natureRock.meshCount > 0;
}

void UnloadOptionalNatureModels() {
    if (treeModelReady) UnloadModel(natureTree);
    if (rockModelReady) UnloadModel(natureRock);
    treeModelReady = false;
    rockModelReady = false;
}

void InitCamera() {
    camera.position = V3(player.position.x, 7.0f, player.position.z + 11.0f);
    camera.target = V3(player.position.x, 1.0f, player.position.z);
    camera.up = V3(0.0f, 1.0f, 0.0f);
    camera.fovy = 52.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void DrawTree(const Prop& p) {
    const float s = p.scale;
    if (treeModelReady) {
        DrawModel(natureTree, V3(p.position.x, 0.0f, p.position.z), 4.0f*s, WHITE);
        return;
    }
    const Biome b = GetBiome(p.position.x);
    const Color leaves = b == Biome::Ember ? Color{ 125, 67, 45, 255 }
                       : (b == Biome::Frost ? Color{ 99, 157, 177, 255 }
                       : (b == Biome::Dune ? Color{ 149, 127, 71, 255 } : Color{ 44, 94, 57, 255 }));
    const float trunkH = (p.kind == 3 ? 2.1f : 3.0f)*s;
    DrawCylinder(V3(p.position.x, trunkH*0.5f, p.position.z), 0.24f*s, 0.36f*s, trunkH, 7,
                 p.kind == 3 ? Color{ 80, 61, 53, 255 } : BROWN);
    if (p.kind == 3) {
        DrawCylinderEx(V3(p.position.x, trunkH, p.position.z),
                       V3(p.position.x+0.9f*s, trunkH+0.65f*s, p.position.z+0.15f*s),
                       0.13f*s, 0.035f*s, 5, Color{ 93, 69, 59, 255 });
        DrawCylinderEx(V3(p.position.x, trunkH*0.75f, p.position.z),
                       V3(p.position.x-0.8f*s, trunkH+0.45f*s, p.position.z-0.2f*s),
                       0.12f*s, 0.025f*s, 5, Color{ 93, 69, 59, 255 });
        return;
    }
    DrawSphere(V3(p.position.x, trunkH+0.5f*s, p.position.z), 1.05f*s, leaves);
    DrawSphere(V3(p.position.x-0.55f*s, trunkH+0.05f*s, p.position.z+0.18f*s), 0.77f*s, leaves);
    DrawSphere(V3(p.position.x+0.58f*s, trunkH+0.12f*s, p.position.z-0.08f*s), 0.80f*s, leaves);
}
void DrawRock(const Prop& p) {
    const float s = p.scale;
    if (rockModelReady) {
        DrawModel(natureRock, V3(p.position.x, 0.15f, p.position.z), 4.2f*s, WHITE);
        return;
    }
    const Color col = GetBiome(p.position.x) == Biome::Frost ? Color{ 150, 177, 185, 255 }
                    : (GetBiome(p.position.x) == Biome::Ember ? Color{ 97, 69, 64, 255 }
                    : Color{ 104, 112, 109, 255 });
    DrawCube(V3(p.position.x, 0.4f*s, p.position.z), 1.45f*s, 0.85f*s, 1.2f*s, col);
    DrawSphere(V3(p.position.x+0.22f*s, 0.55f*s, p.position.z-0.08f*s), 0.53f*s, col);
}
void DrawEnvironment() {
    const float tile = 16.0f;
    for (int ix = -3; ix < 16; ++ix) {
        for (int iz = -4; iz < 4; ++iz) {
            const float x = ix*tile + tile*0.5f;
            const float z = iz*tile + tile*0.5f;
            DrawCube(V3(x, -0.22f, z), tile, 0.44f, tile, GroundColor(GetBiome(x), (ix+iz+40)%3));
        }
    }
    // Soft trails point toward the next mission while keeping the map open for exploration.
    for (float x=-32.0f; x<238.0f; x+=4.0f) {
        const Biome b = GetBiome(x);
        const Color trail = b == Biome::Dune ? Color{ 214, 184, 125, 255 }
                          : (b == Biome::Frost ? Color{ 183, 212, 220, 255 }
                          : (b == Biome::Ember ? Color{ 149, 93, 68, 255 } : Color{ 90, 132, 75, 255 }));
        DrawCube(V3(x, 0.015f, 0.0f), 3.9f, 0.025f, 4.0f, trail);
    }
    for (const Prop& p : props) {
        if (DistXZ(p.position, player.position) > 84.0f) continue;
        switch (p.kind) {
            case 0: case 3: DrawTree(p); break;
            case 1: DrawRock(p); break;
            case 2:
                DrawCylinder(V3(p.position.x, 0.8f*p.scale, p.position.z), 0.14f*p.scale, 0.36f*p.scale,
                             1.6f*p.scale, 5, GetBiome(p.position.x)==Biome::Frost ? C_CYAN : C_VIOLET);
                DrawSphere(V3(p.position.x, 1.75f*p.scale, p.position.z), 0.28f*p.scale, C_CYAN);
                break;
            default:
                DrawSphere(V3(p.position.x, 0.32f*p.scale, p.position.z), 0.52f*p.scale,
                           GetBiome(p.position.x)==Biome::Ember ? Color{ 148, 69, 54, 255 } : Color{ 47, 127, 73, 255 });
                break;
        }
    }
    // Quest altars and distant landmarks.
    for (int i=0; i<BOSS_COUNT; ++i) {
        const Vector3 b = bosses[i].position;
        const bool unlocked = (i==0 ? player.kills >= 3 : !bosses[i-1].alive);
        const Color marker = unlocked ? BossColor(i) : Color{ 110, 119, 137, 255 };
        DrawCylinder(V3(b.x, 0.17f, b.z), 1.25f, 1.25f, 0.34f, 12, Color{ 35, 40, 55, 255 });
        DrawCylinder(V3(b.x, 0.34f, b.z), 0.83f, 0.83f, 0.18f, 12, marker);
        DrawCylinderEx(V3(b.x, 0.8f, b.z), V3(b.x, 3.3f + std::sin(worldClock*2.0f+i)*0.25f, b.z),
                       0.06f, 0.015f, 7, WithAlpha(marker, 160));
        DrawSphere(V3(b.x, 3.45f + std::sin(worldClock*2.0f+i)*0.25f, b.z), 0.2f, marker);
    }
}

void DrawPlayerModel() {
    const Vector3 p = player.position;
    const float step = std::sin(player.walkClock*10.5f) * 0.23f;
    const float bodyBob = player.grounded ? std::fabs(std::sin(player.walkClock*10.5f))*0.05f : 0.0f;
    Color armor = player.hurtFlash > 0.0f ? WHITE : Color{ 65, 109, 164, 255 };
    Color cloth = Color{ 32, 54, 89, 255 };
    // Tiny procedural hero model is the fallback while the real rigged model is being integrated.
    DrawCube(V3(p.x, p.y+1.10f+bodyBob, p.z), 0.56f, 0.78f, 0.36f, armor);
    DrawCube(V3(p.x, p.y+0.64f+bodyBob, p.z-0.02f), 0.62f, 0.32f, 0.41f, cloth);
    DrawSphere(V3(p.x, p.y+1.75f+bodyBob, p.z), 0.25f, Color{ 226, 188, 151, 255 });
    DrawCylinder(V3(p.x, p.y+2.00f+bodyBob, p.z), 0.15f, 0.22f, 0.12f, 7, C_GOLD);
    const Vector3 leftHip = V3(p.x-0.18f, p.y+0.58f, p.z);
    const Vector3 rightHip = V3(p.x+0.18f, p.y+0.58f, p.z);
    DrawCylinderEx(leftHip, V3(p.x-0.18f+step, p.y+0.10f, p.z), 0.105f, 0.08f, 6, Color{ 49, 65, 83, 255 });
    DrawCylinderEx(rightHip, V3(p.x+0.18f-step, p.y+0.10f, p.z), 0.105f, 0.08f, 6, Color{ 49, 65, 83, 255 });
    DrawCylinderEx(V3(p.x-0.32f, p.y+1.36f, p.z), V3(p.x-0.42f, p.y+0.92f, p.z-0.04f), 0.095f, 0.07f, 6, armor);
    DrawCylinderEx(V3(p.x+0.32f, p.y+1.36f, p.z), V3(p.x+0.42f, p.y+0.92f, p.z-0.04f), 0.095f, 0.07f, 6, armor);
    Vector3 hand = V3(p.x+0.42f, p.y+0.94f, p.z-0.06f);
    Vector3 swordTip = V3(hand.x+player.facing.x*0.95f, hand.y+0.28f, hand.z+player.facing.z*0.95f);
    if (player.attackClock > 0.0f) {
        const float swing = (0.5f-player.attackClock)*3.2f;
        swordTip.x += std::sin(swing*PI)*0.5f;
        swordTip.y += std::sin(swing*PI)*0.4f;
        swordTip.z -= std::fabs(std::sin(swing*PI))*0.45f;
    }
    DrawCylinderEx(hand, swordTip, 0.055f, 0.018f, 6, C_GOLD);
    DrawCylinderEx(V3(hand.x, hand.y-0.12f, hand.z), V3(hand.x, hand.y+0.12f, hand.z),
                   0.07f, 0.07f, 6, Color{ 100, 63, 42, 255 });
    if (player.attackClock > 0.0f) {
        DrawCylinderEx(V3(p.x+player.facing.x*0.55f, p.y+1.0f, p.z+player.facing.z*0.55f),
                       V3(p.x+player.facing.x*2.5f, p.y+1.0f, p.z+player.facing.z*2.5f),
                       0.10f, 0.02f, 7, WithAlpha(C_CYAN, 175));
    }
    if (player.abilityClock > 0.0f) {
        DrawSphere(V3(p.x, p.y+1.0f, p.z), 1.35f, WithAlpha(C_CYAN, 35));
        DrawCircle3D(V3(p.x, 0.12f, p.z), 1.8f + std::sin(worldClock*15.0f)*0.15f,
                     V3(1.0f, 0.0f, 0.0f), 90.0f, WithAlpha(C_CYAN, 205));
    }
}

void DrawMonsterModel(Vector3 p, int kind, float phase, float scale, Color color) {
    const float bob = std::sin(phase*4.2f)*0.08f;
    DrawSphere(V3(p.x, p.y+0.78f*scale+bob, p.z), 0.58f*scale, color);
    DrawSphere(V3(p.x, p.y+1.35f*scale+bob, p.z-0.13f*scale), 0.37f*scale, color);
    DrawSphere(V3(p.x-0.14f*scale, p.y+1.42f*scale+bob, p.z-0.45f*scale), 0.06f*scale, WHITE);
    DrawSphere(V3(p.x+0.14f*scale, p.y+1.42f*scale+bob, p.z-0.45f*scale), 0.06f*scale, WHITE);
    DrawSphere(V3(p.x-0.14f*scale, p.y+1.42f*scale+bob, p.z-0.49f*scale), 0.026f*scale, RED);
    DrawSphere(V3(p.x+0.14f*scale, p.y+1.42f*scale+bob, p.z-0.49f*scale), 0.026f*scale, RED);
    const float hornDir = (kind%2==0) ? 1.0f : -1.0f;
    DrawCylinderEx(V3(p.x-0.22f*scale, p.y+1.55f*scale+bob, p.z-0.12f*scale),
                   V3(p.x-0.38f*scale*hornDir, p.y+1.95f*scale+bob, p.z-0.05f*scale),
                   0.10f*scale, 0.01f*scale, 5, Color{ 232, 221, 190, 255 });
    DrawCylinderEx(V3(p.x+0.22f*scale, p.y+1.55f*scale+bob, p.z-0.12f*scale),
                   V3(p.x+0.38f*scale*hornDir, p.y+1.95f*scale+bob, p.z-0.05f*scale),
                   0.10f*scale, 0.01f*scale, 5, Color{ 232, 221, 190, 255 });
    DrawCylinderEx(V3(p.x-0.30f*scale, p.y+0.75f*scale, p.z), V3(p.x-0.44f*scale, p.y+0.13f*scale, p.z+0.05f),
                   0.14f*scale, 0.07f*scale, 6, color);
    DrawCylinderEx(V3(p.x+0.30f*scale, p.y+0.75f*scale, p.z), V3(p.x+0.44f*scale, p.y+0.13f*scale, p.z+0.05f),
                   0.14f*scale, 0.07f*scale, 6, color);
}
void DrawBossModel(int i) {
    Boss& b = bosses[i];
    const float scale = 1.65f + i*0.22f;
    const Color c = BossColor(i);
    const float pulse = 1.0f + std::sin(worldClock*3.0f+i)*0.035f;
    DrawMonsterModel(b.position, i, b.phase, scale*pulse, c);
    DrawSphere(V3(b.position.x, b.position.y+1.65f*scale, b.position.z-0.32f*scale),
               0.28f*scale, WithAlpha(i==2 ? C_GOLD : C_CYAN, 225));
    if (DistXZ(player.position, b.position) < 26.0f) {
        DrawCylinder(V3(b.position.x, 0.05f, b.position.z), 2.15f*scale, 2.15f*scale, 0.05f, 32,
                     WithAlpha(c, 95));
    }
}
void DrawHealthBar3D(Vector3 pos, float health, float maxHealth, float width, Color color) {
    const float ratio = ClampF(health/maxHealth, 0.0f, 1.0f);
    DrawCube(V3(pos.x, pos.y, pos.z), width, 0.10f, 0.08f, Color{ 26, 29, 39, 255 });
    if (ratio > 0.0f)
        DrawCube(V3(pos.x-width*0.5f+width*ratio*0.5f, pos.y+0.015f, pos.z-0.05f),
                 width*ratio, 0.075f, 0.07f, color);
}
void DrawWorldEntities() {
    for (const Monster& m : monsters) {
        if (!m.alive || DistXZ(player.position, m.position)>75.0f) continue;
        DrawMonsterModel(m.position, m.kind, m.phase, 0.76f, MonsterColor(m.kind));
        DrawHealthBar3D(V3(m.position.x, 2.35f, m.position.z), m.health, m.maxHealth, 1.15f, C_RED);
    }
    for (int i=0; i<BOSS_COUNT; ++i) {
        if (bosses[i].alive) DrawBossModel(i);
    }
    for (const Particle& p : particles) {
        const float alpha = ClampF(p.life / std::max(0.01f, p.maxLife), 0.0f, 1.0f);
        DrawSphere(p.position, p.size*alpha, WithAlpha(p.color, static_cast<unsigned char>(255.0f*alpha)));
    }
}

int NextBoss() {
    for (int i=0; i<BOSS_COUNT; ++i) if (bosses[i].alive) return i;
    return BOSS_COUNT;
}
bool BossUnlocked(int i) {
    if (i == 0) return player.kills >= 3;
    return !bosses[i-1].alive;
}
void SetToast(const std::string& message, float time=4.0f) {
    missionToast = message;
    missionToastClock = time;
}
void AddDamageToPlayer(float damage) {
    if (player.invulnerable > 0.0f || screen != Screen::Playing) return;
    player.health = std::max(0.0f, player.health-damage);
    player.invulnerable = 0.9f;
    player.hurtFlash = 0.35f;
    screenShake = std::max(screenShake, 0.16f);
    SpawnBurst(V3(player.position.x, 1.0f, player.position.z), C_RED, 18, 4.4f);
    PlaySfx(sfxPlayerHurt);
    if (player.health <= 0.0f) {
        screen = Screen::Rebirth;
        player.rebirthClock = 1.65f;
        SetToast("REBIRTH — RISING FROM THE ASHES", 2.5f);
    }
}
void KillMonster(Monster& m) {
    if (!m.alive) return;
    m.alive = false;
    player.kills++;
    player.xp += 25;
    SpawnBurst(V3(m.position.x, 1.0f, m.position.z), MonsterColor(m.kind), 32, 5.0f);
    SpawnBurst(V3(m.position.x, 1.0f, m.position.z), C_GOLD, 10, 2.7f);
    PlaySfx(sfxMonsterDown);
    if (player.kills == 3 && bosses[0].alive) SetToast("STONEFANG AWAKENED — FIND THE RED ALTAR", 5.0f);
    else SetToast("MONSTER DEFEATED  +25 XP", 1.6f);
}
void KillBoss(int i) {
    bosses[i].alive = false;
    player.defeatedBosses++;
    player.xp += 300 + i*100;
    player.health = std::min(player.maxHealth, player.health+35.0f);
    SpawnBurst(V3(bosses[i].position.x, 1.2f, bosses[i].position.z), BossColor(i), 75, 7.5f);
    SpawnBurst(V3(bosses[i].position.x, 2.4f, bosses[i].position.z), C_GOLD, 42, 5.0f);
    PlaySfx(sfxBossRoar);
    currentBoss = NextBoss();
    if (currentBoss >= BOSS_COUNT) {
        screen = Screen::Victory;
        SetToast("THE WILDS ARE FREE", 8.0f);
    } else {
        SetToast(std::string(bosses[i].name)+" DEFEATED — NEXT: "+bosses[currentBoss].name, 5.0f);
    }
}
void PlayerAttack() {
    if (player.attackCooldown > 0.0f) return;
    player.attackCooldown = 0.42f;
    player.attackClock = 0.28f;
    player.attackHit = false;
    attackSlashClock = 0.23f;
    SpawnBurst(V3(player.position.x+player.facing.x*1.0f, 1.0f, player.position.z+player.facing.z*1.0f), C_CYAN, 10, 2.3f);
    PlaySfx(sfxHit);
    for (Monster& m : monsters) {
        if (!m.alive) continue;
        const Vector3 to = NormalizeXZ(V3(m.position.x-player.position.x, 0.0f, m.position.z-player.position.z));
        const float dot = to.x*player.facing.x + to.z*player.facing.z;
        if (DistXZ(player.position,m.position) < 3.0f && dot > -0.15f) {
            m.health -= 34.0f;
            SpawnBurst(V3(m.position.x, 1.1f, m.position.z), C_GOLD, 13, 3.2f);
            if (m.health <= 0.0f) KillMonster(m);
        }
    }
    for (int i=0; i<BOSS_COUNT; ++i) {
        Boss& b = bosses[i];
        if (!b.alive || !BossUnlocked(i)) continue;
        const Vector3 to = NormalizeXZ(V3(b.position.x-player.position.x, 0.0f, b.position.z-player.position.z));
        const float dot = to.x*player.facing.x + to.z*player.facing.z;
        if (DistXZ(player.position,b.position) < 4.0f && dot > -0.2f) {
            b.health -= 27.0f;
            SpawnBurst(V3(b.position.x, 1.5f, b.position.z), C_GOLD, 20, 4.2f);
            screenShake = std::max(screenShake, 0.10f);
            if (b.health <= 0.0f) KillBoss(i);
        }
    }
}
void PlayerAbility() {
    if (player.abilityCooldown > 0.0f) return;
    player.abilityCooldown = 5.0f;
    player.abilityClock = 0.72f;
    player.velocity.y = std::max(player.velocity.y, 3.0f);
    player.invulnerable = std::max(player.invulnerable, 0.45f);
    abilityGlowClock = 0.6f;
    SpawnBurst(V3(player.position.x, 1.0f, player.position.z), C_CYAN, 48, 6.5f);
    SpawnBurst(V3(player.position.x, 1.8f, player.position.z), C_VIOLET, 25, 4.6f);
    PlaySfx(sfxAbility);
    for (Monster& m : monsters) {
        if (!m.alive || DistXZ(player.position, m.position)>5.2f) continue;
        m.health -= 65.0f;
        SpawnBurst(V3(m.position.x, 1.0f, m.position.z), C_CYAN, 20, 4.0f);
        if (m.health <= 0.0f) KillMonster(m);
    }
    for (int i=0; i<BOSS_COUNT; ++i) {
        Boss& b = bosses[i];
        if (!b.alive || !BossUnlocked(i) || DistXZ(player.position,b.position)>5.8f) continue;
        b.health -= 42.0f;
        SpawnBurst(V3(b.position.x, 1.6f, b.position.z), C_VIOLET, 27, 4.7f);
        if (b.health <= 0.0f) KillBoss(i);
    }
}

void UpdatePlayer(float dt) {
    if (player.attackCooldown > 0.0f) player.attackCooldown -= dt;
    if (player.attackClock > 0.0f) player.attackClock -= dt;
    if (player.abilityCooldown > 0.0f) player.abilityCooldown -= dt;
    if (player.abilityClock > 0.0f) player.abilityClock -= dt;
    if (player.invulnerable > 0.0f) player.invulnerable -= dt;
    if (player.hurtFlash > 0.0f) player.hurtFlash -= dt;
    if (screenShake > 0.0f) screenShake -= dt;
    if (attackSlashClock > 0.0f) attackSlashClock -= dt;
    if (abilityGlowClock > 0.0f) abilityGlowClock -= dt;

    const int sw = GetScreenWidth(), sh = GetScreenHeight();
    const float uiScale = std::max(0.75f, std::min(sw/960.0f, sh/540.0f));
    const Rectangle joy = ButtonRect(16.0f*uiScale, sh-164.0f*uiScale, 158.0f*uiScale, 148.0f*uiScale);
    Vector3 move = V3(0.0f, 0.0f, 0.0f);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) move.x += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) move.x -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move.z += 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move.z -= 1.0f;
    const int touchCount = GetTouchPointCount();
    for (int i=0; i<touchCount; ++i) {
        const Vector2 t = GetTouchPosition(i);
        if (CheckCollisionPointRec(t, joy)) {
            const Vector2 center = { joy.x+joy.width*0.5f, joy.y+joy.height*0.5f };
            const Vector2 delta = { t.x-center.x, t.y-center.y };
            move.x += ClampF(delta.x/(joy.width*0.43f), -1.0f, 1.0f);
            move.z += ClampF(delta.y/(joy.height*0.43f), -1.0f, 1.0f);
        }
    }
    const float len = std::sqrt(move.x*move.x+move.z*move.z);
    if (len > 1.0f) { move.x/=len; move.z/=len; }
    if (len > 0.1f) {
        player.facing = NormalizeXZ(move);
        player.walkClock += dt;
    }
    const float speed = player.abilityClock > 0.0f ? 12.0f : 7.0f;
    player.position.x += move.x*speed*dt;
    player.position.z += move.z*speed*dt;
    player.position.x = ClampF(player.position.x, WORLD_MIN_X, WORLD_MAX_X);
    player.position.z = ClampF(player.position.z, -WORLD_HALF_Z+2.0f, WORLD_HALF_Z-2.0f);

    const Rectangle jumpButton = ButtonRect(sw-106*uiScale, sh-205*uiScale, 54*uiScale, 42*uiScale);
    if ((IsKeyPressed(KEY_SPACE) || Pressed(jumpButton)) && player.grounded) {
        player.velocity.y = 8.3f;
        player.grounded = false;
        SpawnBurst(V3(player.position.x, 0.15f, player.position.z), C_GOLD, 8, 1.8f);
    }
    player.velocity.y -= 20.0f*dt;
    player.position.y += player.velocity.y*dt;
    if (player.position.y <= 0.0f) {
        if (!player.grounded && player.velocity.y < -5.0f)
            SpawnBurst(V3(player.position.x, 0.12f, player.position.z), Color{ 190, 194, 161, 255 }, 8, 1.4f);
        player.position.y = 0.0f;
        player.velocity.y = 0.0f;
        player.grounded = true;
    }

    if (IsKeyPressed(KEY_J) || Pressed(ButtonRect(sw-104*uiScale, sh-104*uiScale, 90*uiScale, 90*uiScale))) PlayerAttack();
    if (IsKeyPressed(KEY_K) || Pressed(ButtonRect(sw-220*uiScale, sh-154*uiScale, 80*uiScale, 80*uiScale))) PlayerAbility();

    // Camera smoothing creates a stable third-person chase camera on desktop and Android.
    const Vector3 desiredPos = V3(player.position.x-player.facing.x*2.0f, 7.2f+player.position.y, player.position.z+10.5f-player.facing.z*1.5f);
    camera.position = Vector3Lerp(camera.position, desiredPos, ClampF(dt*3.5f, 0.0f, 1.0f));
    camera.target = Vector3Lerp(camera.target, V3(player.position.x, player.position.y+0.95f, player.position.z), ClampF(dt*5.0f, 0.0f, 1.0f));

    if (player.kills >= 3 && bosses[0].alive && !BossUnlocked(0)) {
        SetToast("A HUNTING RITUAL BEGINS", 3.0f);
    }
}

void UpdateEnemies(float dt) {
    for (Monster& m : monsters) {
        if (!m.alive) continue;
        m.phase += dt;
        m.attackClock -= dt;
        const float distance = DistXZ(player.position, m.position);
        if (distance < 21.0f && distance > 1.65f) {
            const Vector3 dir = NormalizeXZ(V3(player.position.x-m.position.x, 0.0f, player.position.z-m.position.z));
            m.position.x += dir.x*m.speed*dt;
            m.position.z += dir.z*m.speed*dt;
        } else if (distance >= 21.0f) {
            const float wobble = std::sin(m.phase*0.65f)*0.28f;
            m.position.x += wobble*dt;
            m.position.z += std::cos(m.phase*0.9f)*0.3f*dt;
            if (DistXZ(m.home,m.position)>5.0f) {
                const Vector3 back = NormalizeXZ(V3(m.home.x-m.position.x,0,m.home.z-m.position.z));
                m.position.x += back.x*dt;
                m.position.z += back.z*dt;
            }
        }
        m.position.x = ClampF(m.position.x, WORLD_MIN_X, WORLD_MAX_X);
        m.position.z = ClampF(m.position.z, -WORLD_HALF_Z+1.0f, WORLD_HALF_Z-1.0f);
        if (distance < 1.8f && m.attackClock <= 0.0f) {
            m.attackClock = 1.55f;
            AddDamageToPlayer(8.0f);
            SpawnBurst(V3(m.position.x, 0.85f, m.position.z), C_RED, 10, 2.5f);
        }
    }

    for (int i=0; i<BOSS_COUNT; ++i) {
        Boss& b = bosses[i];
        if (!b.alive) continue;
        b.phase += dt;
        b.attackClock -= dt;
        if (!BossUnlocked(i)) continue;
        const float distance = DistXZ(player.position,b.position);
        if (distance < 42.0f && distance > 2.9f) {
            const Vector3 dir = NormalizeXZ(V3(player.position.x-b.position.x,0,player.position.z-b.position.z));
            b.position.x += dir.x*b.speed*dt;
            b.position.z += dir.z*b.speed*dt;
        }
        if (distance < 3.4f && b.attackClock <= 0.0f) {
            b.attackClock = 2.1f + i*0.18f;
            AddDamageToPlayer(14.0f + i*3.0f);
            SpawnBurst(V3(player.position.x, 0.35f, player.position.z), BossColor(i), 26, 5.4f);
            screenShake = 0.32f;
            PlaySfx(sfxBossRoar);
        } else if (distance >= 3.4f && distance < 10.0f && b.attackClock <= 0.0f) {
            // Telegraph a short-range shockwave: readable danger cue before damage.
            b.attackClock = 2.8f;
            SpawnBurst(V3(b.position.x, 0.22f, b.position.z), BossColor(i), 18, 2.8f);
            if (distance < 7.0f) AddDamageToPlayer(8.0f + i*2.0f);
        }
    }
}
void UpdateParticles(float dt) {
    for (Particle& p : particles) {
        p.life -= dt;
        p.position.x += p.velocity.x*dt;
        p.position.y += p.velocity.y*dt;
        p.position.z += p.velocity.z*dt;
        p.velocity.y -= 5.5f*dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p){ return p.life <= 0.0f; }), particles.end());
}

void DrawBar(float x, float y, float w, float h, float ratio, Color fill) {
    DrawRectangleRounded({x,y,w,h}, 0.45f, 8, Color{ 20, 25, 39, 220 });
    if (ratio > 0.0f) DrawRectangleRounded({x+2,y+2,(w-4)*ClampF(ratio,0.0f,1.0f),h-4},0.4f,8,fill);
    DrawRectangleRoundedLinesEx({x,y,w,h},0.45f,8,1.4f,Color{ 206, 222, 244, 170 });
}
void DrawHud() {
    const int sw = GetScreenWidth(), sh = GetScreenHeight();
    const float scale = std::max(0.65f, std::min(sw/960.0f, sh/540.0f));
    const float left = 18.0f*scale;
    DrawRectangleRounded({left, 16*scale, 260*scale, 88*scale},0.08f,8,WithAlpha(C_INK,205));
    DrawText("WILDBOUND", static_cast<int>(left+12*scale), static_cast<int>(24*scale), static_cast<int>(20*scale), C_MINT);
    DrawText(TextFormat("LV. %i    %i XP",1+player.xp/250,player.xp), static_cast<int>(left+12*scale), static_cast<int>(48*scale), static_cast<int>(12*scale), RAYWHITE);
    DrawBar(left+12*scale,68*scale,220*scale,14*scale,player.health/player.maxHealth,
            player.health < 30.0f ? C_RED : Color{ 76, 223, 150, 255 });
    DrawText(TextFormat("%.0f / %.0f HP",player.health,player.maxHealth),
             static_cast<int>(left+12*scale),static_cast<int>(84*scale),static_cast<int>(10*scale),RAYWHITE);

    const Biome b = GetBiome(player.position.x);
    const char* region = BiomeName(b);
    int regionWidth = MeasureText(region, static_cast<int>(17*scale));
    DrawText(region, (sw-regionWidth)/2, static_cast<int>(18*scale), static_cast<int>(17*scale), WHITE);
    const int next = NextBoss();
    if (next < BOSS_COUNT) {
        const std::string objective = (next==0 && player.kills<3)
            ? "MISSION 01  |  HUNT SHADES" : std::string("BOSS MISSION  |  ")+bosses[next].name;
        DrawRectangleRounded({sw-278*scale,15*scale,260*scale,53*scale},0.1f,8,WithAlpha(C_INK,205));
        DrawText(objective.c_str(),static_cast<int>(sw-266*scale),static_cast<int>(23*scale),
                 static_cast<int>(12*scale),C_GOLD);
        DrawText(next==0 && player.kills<3 ? TextFormat("SHADES DEFEATED  %i / 3",std::min(player.kills,3))
                                           : bosses[next].mission,
                 static_cast<int>(sw-266*scale),static_cast<int>(43*scale),static_cast<int>(10*scale),RAYWHITE);
    }

    // Boss marker compass and health bar stay visible when in an encounter.
    for (int i=0; i<BOSS_COUNT; ++i) {
        if (!bosses[i].alive || !BossUnlocked(i)) continue;
        const float dist = DistXZ(player.position,bosses[i].position);
        if (dist < 24.0f) {
            const float bw = std::min(sw*0.52f, 480.0f*scale);
            DrawText(bosses[i].name,static_cast<int>((sw-bw)/2),static_cast<int>(sh*0.12f),
                     static_cast<int>(15*scale),BossColor(i));
            DrawBar((sw-bw)/2,sh*0.12f+22*scale,bw,15*scale,bosses[i].health/bosses[i].maxHealth,BossColor(i));
        }
    }
    const float joyX = 16*scale, joyY = sh-164*scale;
    DrawCircle(static_cast<int>(joyX+78*scale),static_cast<int>(joyY+74*scale),68*scale,WithAlpha(Color{ 11,20,36,255 },130));
    DrawCircleLines(static_cast<int>(joyX+78*scale),static_cast<int>(joyY+74*scale),68*scale,WithAlpha(C_MINT,145));
    DrawCircle(static_cast<int>(joyX+78*scale),static_cast<int>(joyY+74*scale),25*scale,WithAlpha(C_MINT,80));
    DrawText("MOVE",static_cast<int>(joyX+50*scale),static_cast<int>(joyY+65*scale),static_cast<int>(12*scale),WHITE);

    const Rectangle attack = ButtonRect(sw-104*scale,sh-104*scale,90*scale,90*scale);
    DrawCircle(static_cast<int>(attack.x+attack.width/2),static_cast<int>(attack.y+attack.height/2),
               attack.width/2,WithAlpha(C_RED,185));
    DrawCircleLines(static_cast<int>(attack.x+attack.width/2),static_cast<int>(attack.y+attack.height/2),
                    attack.width/2,WithAlpha(WHITE,230));
    DrawText("ATTACK",static_cast<int>(attack.x+14*scale),static_cast<int>(attack.y+36*scale),static_cast<int>(13*scale),WHITE);

    const Rectangle skill = ButtonRect(sw-219*scale,sh-154*scale,82*scale,82*scale);
    DrawCircle(static_cast<int>(skill.x+skill.width/2),static_cast<int>(skill.y+skill.height/2),
               skill.width/2,WithAlpha(C_VIOLET,175));
    DrawCircleLines(static_cast<int>(skill.x+skill.width/2),static_cast<int>(skill.y+skill.height/2),
                    skill.width/2,WithAlpha(WHITE,220));
    DrawText("BLINK",static_cast<int>(skill.x+22*scale),static_cast<int>(skill.y+30*scale),static_cast<int>(12*scale),WHITE);
    if (player.abilityCooldown > 0.0f)
        DrawText(TextFormat("%.1f",player.abilityCooldown),static_cast<int>(skill.x+30*scale),static_cast<int>(skill.y+46*scale),
                 static_cast<int>(11*scale),WHITE);
    const Rectangle jump = ButtonRect(sw-106*scale,sh-205*scale,54*scale,42*scale);
    DrawRectangleRounded(jump,0.25f,8,WithAlpha(C_CYAN,155));
    DrawText("JUMP",static_cast<int>(jump.x+9*scale),static_cast<int>(jump.y+13*scale),static_cast<int>(10*scale),C_INK);

    DrawRectangleRounded({sw*0.36f,sh-40*scale,sw*0.28f,25*scale},0.2f,6,WithAlpha(C_INK,160));
    DrawText(TextFormat("MONSTERS %i   BOSSES %i/%i   TAB: PAUSE",player.kills,player.defeatedBosses,BOSS_COUNT),
             static_cast<int>(sw*0.36f+10*scale),static_cast<int>(sh-34*scale),static_cast<int>(11*scale),WHITE);
    if (missionToastClock > 0.0f) {
        const float alpha = ClampF(missionToastClock*2.0f,0.0f,1.0f);
        const int tw = MeasureText(missionToast.c_str(),static_cast<int>(15*scale));
        DrawRectangleRounded({(sw-tw-44*scale)/2,sh*0.19f,tw+44*scale,35*scale},0.3f,8,WithAlpha(C_INK,static_cast<unsigned char>(220*alpha)));
        DrawText(missionToast.c_str(),(sw-tw)/2,static_cast<int>(sh*0.19f+10*scale),static_cast<int>(15*scale),WithAlpha(C_GOLD,static_cast<unsigned char>(255*alpha)));
    }
}

void DrawMenu() {
    const int sw=GetScreenWidth(), sh=GetScreenHeight();
    DrawRectangle(0,0,sw,sh,WithAlpha(C_INK,148));
    const float scale=std::max(0.7f,std::min(sw/960.0f,sh/540.0f));
    DrawText("WILDBOUND",static_cast<int>(sw*0.13f),static_cast<int>(sh*0.24f),
             static_cast<int>(56*scale),C_MINT);
    DrawText("OPEN-WORLD MONSTER HUNT",static_cast<int>(sw*0.135f),static_cast<int>(sh*0.38f),
             static_cast<int>(19*scale),C_GOLD);
    DrawText("Explore four biomes. Hunt creatures. Defeat four ancient bosses.",
             static_cast<int>(sw*0.135f),static_cast<int>(sh*0.45f),static_cast<int>(13*scale),RAYWHITE);
    const Rectangle start={sw*0.135f,sh*0.57f,sw*0.30f,52*scale};
    DrawRectangleRounded(start,0.18f,8,WithAlpha(C_MINT,220));
    DrawText("START HUNT",static_cast<int>(start.x+26*scale),static_cast<int>(start.y+13*scale),
             static_cast<int>(20*scale),C_INK);
    DrawText("WASD / ARROWS: MOVE   SPACE: JUMP   J: ATTACK   K: SPECIAL",
             static_cast<int>(sw*0.135f),static_cast<int>(sh*0.75f),static_cast<int>(12*scale),Color{210,224,240,255});
    DrawText("Offline prototype | Procedural fallback art | No server required",
             static_cast<int>(sw*0.135f),static_cast<int>(sh*0.80f),static_cast<int>(11*scale),Color{158,177,202,255});
}
void DrawOverlay(const char* title, const char* subtitle, bool showResume) {
    const int sw=GetScreenWidth(), sh=GetScreenHeight();
    DrawRectangle(0,0,sw,sh,WithAlpha(C_INK,190));
    const float scale=std::max(0.7f,std::min(sw/960.0f,sh/540.0f));
    const int titleSize=static_cast<int>(42*scale);
    DrawText(title,(sw-MeasureText(title,titleSize))/2,static_cast<int>(sh*0.36f),titleSize,C_MINT);
    const int subSize=static_cast<int>(15*scale);
    DrawText(subtitle,(sw-MeasureText(subtitle,subSize))/2,static_cast<int>(sh*0.48f),subSize,RAYWHITE);
    if(showResume) {
        const char* label="TAP / ENTER TO CONTINUE";
        DrawText(label,(sw-MeasureText(label,static_cast<int>(13*scale)))/2,static_cast<int>(sh*0.60f),
                 static_cast<int>(13*scale),C_GOLD);
    }
}

void ResetGame() {
    MakeWorld();
    InitCamera();
    screen = Screen::Playing;
}
} // namespace

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(DESIGN_W, DESIGN_H, "Wildbound: Boss Hunter");
    SetTargetFPS(60);
    InitAudio();
    MakeWorld();
    InitCamera();
    LoadOptionalNatureModels();

    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.033f);
        const int sw=GetScreenWidth(), sh=GetScreenHeight();
        worldClock += dt;
        menuPulse += dt;
        CaptureNewTouchPresses();
        if (missionToastClock > 0.0f) missionToastClock -= dt;
        UpdateMusic();

        if (screen == Screen::Menu) {
            if (IsKeyPressed(KEY_ENTER) || Pressed({sw*0.13f,sh*0.52f,sw*0.36f,sh*0.18f})) {
                PlaySfx(sfxMenu);
                screen=Screen::Playing;
                SetToast("MISSION 01 — HUNT THREE SHADES",4.0f);
            }
        } else if (screen == Screen::Playing) {
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
                screen=Screen::Paused;
            } else {
                UpdatePlayer(dt);
                UpdateEnemies(dt);
                UpdateParticles(dt);
            }
        } else if (screen == Screen::Paused) {
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
                Pressed({sw*0.28f,sh*0.53f,sw*0.44f,sh*0.18f})) screen=Screen::Playing;
            if (IsKeyPressed(KEY_R)) ResetGame();
            UpdateParticles(dt);
        } else if (screen == Screen::Rebirth) {
            player.rebirthClock -= dt;
            UpdateParticles(dt);
            if (player.rebirthClock <= 0.0f) {
                player.health = player.maxHealth*0.72f;
                player.position.x = ClampF(player.position.x-6.0f,WORLD_MIN_X,WORLD_MAX_X);
                player.position.y=0.0f;
                player.velocity=V3(0,0,0);
                player.invulnerable=1.7f;
                screen=Screen::Playing;
                SpawnBurst(V3(player.position.x,1.0f,player.position.z),C_CYAN,45,5.0f);
                PlaySfx(sfxRebirth);
                SetToast("REBORN — RETURN TO THE HUNT",3.4f);
            }
        } else if (screen == Screen::Victory) {
            UpdateParticles(dt);
            if (IsKeyPressed(KEY_ENTER) || Pressed({sw*0.26f,sh*0.56f,sw*0.48f,sh*0.19f})) {
                ResetGame();
                screen=Screen::Menu;
            }
        }

        const Biome biome=GetBiome(player.position.x);
        const Color sky = biome==Biome::Dune ? Color{ 237, 190, 123, 255 }
                        : (biome==Biome::Frost ? Color{ 143, 185, 212, 255 }
                        : (biome==Biome::Ember ? Color{ 99, 68, 79, 255 } : Color{ 105, 177, 212, 255 }));
        BeginDrawing();
        ClearBackground(sky);
        BeginMode3D(camera);
            DrawEnvironment();
            DrawWorldEntities();
            DrawPlayerModel();
            // A subtle horizon grid gives distance cues without requiring a heavy shader.
            if (DistXZ(player.position,V3(player.position.x,0,0)) < 20.0f) {
                DrawGrid(24,4.0f);
            }
        EndMode3D();
        if (screenShake > 0.0f) {
            DrawRectangle(0,0,sw,sh,WithAlpha(C_RED,static_cast<unsigned char>(screenShake*55.0f/0.32f)));
        }
        if (screen==Screen::Menu) DrawMenu();
        else if(screen==Screen::Playing) {
            DrawHud();
            if(player.hurtFlash>0.0f) DrawRectangle(0,0,sw,sh,WithAlpha(C_RED,static_cast<unsigned char>(ClampF(player.hurtFlash/0.35f,0.0f,1.0f)*40.0f)));
        }
        else if(screen==Screen::Paused) DrawOverlay("PAUSED","The world waits for you.",true);
        else if(screen==Screen::Rebirth) DrawOverlay("REBIRTH","Defeat is not the end. Recovering health...",false);
        else if(screen==Screen::Victory) DrawOverlay("THE WILDS ARE FREE","All four bosses defeated. Press Enter to explore again.",true);
        EndDrawing();
    }

    UnloadOptionalNatureModels();
    CloseAudio();
    CloseWindow();
    return 0;
}

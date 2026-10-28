#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <cctype>
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
    bool moving = false;
    bool running = false;
    bool grounded = true;
    bool attackHit = false;
    int kills = 0;
    int xp = 0;
    int defeatedBosses = 0;
};

Screen screen = Screen::Menu;
Player player;
Camera3D camera = {};
// Third-person orbit camera: right-drag rotates, mouse wheel zooms.
float cameraYaw = 0.0f;
float cameraPitch = 0.46f;
float cameraDistance = 12.0f;
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
struct CharacterAnimationSet {
    ModelAnimation* clips = nullptr;
    int count = 0;
    int idle = -1;
    int walk = -1;
    int run = -1;
    int attack = -1;
    int hit = -1;
    int death = -1;
    std::vector<float> frameClocks;
    int lastSelected = -1;
};

Model natureTree = {};
Model natureRock = {};
Model sketchfabHeroModel = {};
Model sketchfabMonsterModel = {};
Model sketchfabBossModel = {};
CharacterAnimationSet heroAnimations;
CharacterAnimationSet monsterAnimations;
CharacterAnimationSet bossAnimations;
float heroGroundOffset = 0.0f;
float monsterGroundOffset = 0.0f;
float bossGroundOffset = 0.0f;
Model terrainModel = {};
Texture2D terrainTexture = {};
Model fallbackRockModel = {};
bool terrainReady = false;
bool fallbackRockReady = false;
bool treeModelReady = false;
bool rockModelReady = false;
bool sketchfabHeroReady = false;
bool sketchfabMonsterReady = false;
bool sketchfabBossReady = false;
bool heroModelFacesPositiveZ = false;
bool monsterModelFacesPositiveZ = false;
bool bossModelFacesPositiveZ = false;

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

constexpr float TERRAIN_MIN_X = WORLD_MIN_X - 26.0f;
constexpr float TERRAIN_MAX_X = WORLD_MAX_X + 26.0f;
constexpr float TERRAIN_MIN_Z = -WORLD_HALF_Z - 15.0f;
constexpr float TERRAIN_MAX_Z = WORLD_HALF_Z + 15.0f;

float TerrainHeightAt(float x, float z) {
    const float outer = ClampF((std::fabs(z) - 2.5f)/12.0f, 0.0f, 1.0f);
    const float rolling = (1.55f*std::sin(x*0.027f + std::sin(z*0.052f))
                         + 0.92f*std::sin(z*0.083f + x*0.019f)
                         + 0.55f*std::cos(x*0.061f - z*0.043f))*(0.28f + 0.82f*outer);
    float height = 2.65f + rolling;
    switch (GetBiome(x)) {
        case Biome::Emerald:
            height += 0.25f*std::sin(x*0.12f + z*0.07f);
            break;
        case Biome::Dune:
            height = 2.35f + 0.72f*std::sin(x*0.023f + std::sin(z*0.042f))
                   + 0.35f*std::sin(z*0.085f + x*0.012f);
            break;
        case Biome::Frost:
            height = 3.45f + 1.25f*std::fabs(std::sin(x*0.047f + z*0.018f))
                   + 0.70f*std::sin(z*0.074f + x*0.021f);
            break;
        case Biome::Ember:
            height = 2.20f + 1.1f*std::sin(x*0.031f + z*0.028f)
                   + 0.58f*std::sin(z*0.095f - x*0.017f);
            break;
    }
    return std::max(0.25f, height);
}

Color BlendColor(Color a, Color b, float t) {
    t = ClampF(t, 0.0f, 1.0f);
    return Color{
        static_cast<unsigned char>(a.r + (b.r - a.r)*t),
        static_cast<unsigned char>(a.g + (b.g - a.g)*t),
        static_cast<unsigned char>(a.b + (b.b - a.b)*t), 255
    };
}

float SurfaceNoise(float x, float z) {
    return 0.5f + 0.22f*std::sin(x*0.13f + std::sin(z*0.21f))
                + 0.17f*std::cos(z*0.29f - x*0.071f)
                + 0.08f*std::sin(x*0.91f + z*0.77f)
                + 0.035f*std::cos(x*2.8f - z*2.3f);
}

Image GenerateTerrainAlbedo() {
    constexpr int width = 1536, height = 512;
    Image image = GenImageColor(width, height, Color{72, 88, 62, 255});
    if (image.data == nullptr) return image;
    Color* pixels = static_cast<Color*>(image.data);
    const float spanX = TERRAIN_MAX_X - TERRAIN_MIN_X;
    const float spanZ = TERRAIN_MAX_Z - TERRAIN_MIN_Z;
    for (int py = 0; py < height; ++py) {
        const float worldZ = TERRAIN_MIN_Z + (static_cast<float>(py)/(height-1))*spanZ;
        for (int px = 0; px < width; ++px) {
            const float worldX = TERRAIN_MIN_X + (static_cast<float>(px)/(width-1))*spanX;
            const float noise = SurfaceNoise(worldX, worldZ);
            Color base;
            Color soil;
            switch (GetBiome(worldX)) {
                case Biome::Emerald:
                    base = Color{78, 100, 66, 255}; soil = Color{105, 91, 65, 255}; break;
                case Biome::Dune:
                    base = Color{159, 137, 96, 255}; soil = Color{128, 105, 70, 255}; break;
                case Biome::Frost:
                    base = Color{126, 146, 148, 255}; soil = Color{105, 111, 105, 255}; break;
                default:
                    base = Color{91, 70, 64, 255}; soil = Color{113, 74, 57, 255}; break;
            }
            const float brightness = 0.74f + noise*0.56f;
            Color color = Color{
                static_cast<unsigned char>(ClampF(base.r*brightness, 0, 255)),
                static_cast<unsigned char>(ClampF(base.g*brightness, 0, 255)),
                static_cast<unsigned char>(ClampF(base.b*brightness, 0, 255)), 255
            };
            const float trailCenter = 1.15f*std::sin(worldX*0.034f) + 0.45f*std::sin(worldX*0.081f);
            const float trailDistance = std::fabs(worldZ - trailCenter);
            const float trail = 0.88f*(1.0f - ClampF((trailDistance - 1.35f)/2.2f, 0.0f, 1.0f));
            color = BlendColor(color, soil, trail);
            const float rockNoise = 0.5f + 0.5f*std::sin(worldX*0.18f + std::sin(worldZ*0.21f))*std::cos(worldZ*0.16f-worldX*0.035f);
            if ((GetBiome(worldX) == Biome::Frost || GetBiome(worldX) == Biome::Ember) && rockNoise > 0.81f)
                color = BlendColor(color, Color{76, 77, 76, 255}, (rockNoise-0.81f)*1.65f);
            const float grit = (noise - 0.5f)*20.0f;
            color.r = static_cast<unsigned char>(ClampF(color.r + grit, 0, 255));
            color.g = static_cast<unsigned char>(ClampF(color.g + grit*0.9f, 0, 255));
            color.b = static_cast<unsigned char>(ClampF(color.b + grit*0.8f, 0, 255));
            pixels[py*width + px] = color;
        }
    }
    return image;
}

void LoadProceduralTerrain() {
    if (terrainReady) return;
    constexpr int columns = 280, rows = 120;
    Mesh mesh = {};
    mesh.vertexCount = columns*rows;
    mesh.triangleCount = (columns-1)*(rows-1)*2;
    mesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount*3*sizeof(float))));
    mesh.texcoords = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount*2*sizeof(float))));
    mesh.normals = static_cast<float*>(MemAlloc(static_cast<unsigned int>(mesh.vertexCount*3*sizeof(float))));
    mesh.indices = static_cast<unsigned short*>(MemAlloc(static_cast<unsigned int>(mesh.triangleCount*3*sizeof(unsigned short))));
    if (mesh.vertices == nullptr || mesh.texcoords == nullptr || mesh.normals == nullptr || mesh.indices == nullptr) {
        if (mesh.vertices) MemFree(mesh.vertices);
        if (mesh.texcoords) MemFree(mesh.texcoords);
        if (mesh.normals) MemFree(mesh.normals);
        if (mesh.indices) MemFree(mesh.indices);
        TraceLog(LOG_ERROR, "Could not allocate the 3D terrain mesh.");
        return;
    }
    const float stepX = (TERRAIN_MAX_X - TERRAIN_MIN_X)/(columns-1);
    const float stepZ = (TERRAIN_MAX_Z - TERRAIN_MIN_Z)/(rows-1);
    for (int row = 0; row < rows; ++row) {
        const float z = TERRAIN_MIN_Z + row*stepZ;
        for (int col = 0; col < columns; ++col) {
            const float x = TERRAIN_MIN_X + col*stepX;
            const float y = TerrainHeightAt(x, z);
            const int i = row*columns + col;
            mesh.vertices[i*3] = x;
            mesh.vertices[i*3+1] = y;
            mesh.vertices[i*3+2] = z;
            mesh.texcoords[i*2] = static_cast<float>(col)/(columns-1);
            mesh.texcoords[i*2+1] = static_cast<float>(row)/(rows-1);
            const float dx = (TerrainHeightAt(x+0.25f, z)-TerrainHeightAt(x-0.25f, z))*2.0f;
            const float dz = (TerrainHeightAt(x, z+0.25f)-TerrainHeightAt(x, z-0.25f))*2.0f;
            const Vector3 normal = Vector3Normalize(V3(-dx, 1.0f, -dz));
            mesh.normals[i*3] = normal.x;
            mesh.normals[i*3+1] = normal.y;
            mesh.normals[i*3+2] = normal.z;
        }
    }
    int tri = 0;
    for (int row = 0; row < rows-1; ++row) {
        for (int col = 0; col < columns-1; ++col) {
            const unsigned short a = static_cast<unsigned short>(row*columns + col);
            const unsigned short b = static_cast<unsigned short>((row+1)*columns + col);
            const unsigned short c = static_cast<unsigned short>(row*columns + col + 1);
            const unsigned short d = static_cast<unsigned short>((row+1)*columns + col + 1);
            mesh.indices[tri++] = a; mesh.indices[tri++] = b; mesh.indices[tri++] = c;
            mesh.indices[tri++] = c; mesh.indices[tri++] = b; mesh.indices[tri++] = d;
        }
    }
    UploadMesh(&mesh, false);
    terrainModel = LoadModelFromMesh(mesh);
    Image albedo = GenerateTerrainAlbedo();
    if (albedo.data != nullptr) {
        terrainTexture = LoadTextureFromImage(albedo);
        UnloadImage(albedo);
        if (terrainModel.materialCount > 0) {
            terrainModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = terrainTexture;
            terrainModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        }
        SetTextureFilter(terrainTexture, TEXTURE_FILTER_TRILINEAR);
    }
    fallbackRockModel = LoadModelFromMesh(GenMeshSphere(1.0f, 9, 7));
    fallbackRockReady = fallbackRockModel.meshCount > 0;
    terrainReady = terrainModel.meshCount > 0;
    TraceLog(LOG_INFO, "3D terrain ready: %s, %i vertices, textured biome ground",
             terrainReady ? "mesh loaded" : "procedural fallback", mesh.vertexCount);
}

void UnloadProceduralTerrain() {
    if (terrainReady) {
        // This texture is owned separately so detach it before unloading the model.
        if (terrainModel.materialCount > 0)
            terrainModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = Texture2D{};
        UnloadModel(terrainModel);
    }
    if (terrainTexture.id > 0) UnloadTexture(terrainTexture);
    if (fallbackRockReady) UnloadModel(fallbackRockModel);
    terrainModel = Model{};
    terrainTexture = Texture2D{};
    fallbackRockModel = Model{};
    terrainReady = false;
    fallbackRockReady = false;
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
void LoadRealSound(Sound& target, const char* basePath) {
    const char* extensions[] = { ".wav", ".ogg", ".mp3", ".flac" };
    for (const char* extension : extensions) {
        const std::string path = std::string(basePath) + extension;
        if (!FileExists(path.c_str())) continue;
        Sound replacement = LoadSound(path.c_str());
        if (replacement.frameCount == 0) continue;
        if (target.frameCount > 0) UnloadSound(target);
        target = replacement;
        TraceLog(LOG_INFO, "Loaded game sound: %s", path.c_str());
        return;
    }
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
        if (std::fabs(z) < 5.5f && (x > -25 && x < 225)) continue;
        const float scale = static_cast<float>(GetRandomValue(70, 155)) / 100.0f;
        const int kindRoll = GetRandomValue(0, 99);
        int kind = kindRoll < 50 ? 0 : (kindRoll < 76 ? 1 : (kindRoll < 88 ? 2 : 4));
        if (x > 48 && x < 112 && kind == 0) kind = 1;
        if (x > 112 && x < 174 && kind == 0) kind = 3;
        props.push_back({ V3(x, TerrainHeightAt(x, z), z), scale, kind, GetRandomValue(0, 3) });
    }
    for (int i = 0; i < 13; ++i) {
        const float x = static_cast<float>(GetRandomValue(-8, 228));
        const float z = static_cast<float>(GetRandomValue(-35, 35));
        const float groundY = TerrainHeightAt(x, z);
        monsters.push_back({ V3(x, groundY, z), V3(x, groundY, z),
                             48.0f, 48.0f, 1.7f + GetRandomValue(0, 12)/10.0f,
                             static_cast<float>(GetRandomValue(3, 7))/2.0f, 0.0f, true, i%4 });
    }
    for (int i=0; i<BOSS_COUNT; ++i) {
        bosses[i].position.y = TerrainHeightAt(bosses[i].position.x, bosses[i].position.z);
        bosses[i].health = bosses[i].maxHealth;
        bosses[i].alive = true;
        bosses[i].attackClock = 1.6f + i*0.35f;
        bosses[i].phase = 0.0f;
    }
    player = Player{};
    player.position = V3(-16.0f, TerrainHeightAt(-16.0f, 0.0f), 0.0f);
    currentBoss = 0;
    missionToast = "EXPLORE THE WILDS";
    missionToastClock = 4.5f;
    worldClock = 0.0f;
}

bool IsAbsoluteAssetPath(const std::string& path) {
    if (path.empty()) return false;
#if defined(_WIN32)
    return (path.size() >= 3 && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
        || (path.size() >= 2 && (path[0] == '/' || path[0] == '\\') && path[1] == path[0]);
#else
    return path[0] == '/';
#endif
}

// Resolve assets beside the executable first. Relative paths based on the
// process working directory break when a Windows build is launched from a
// shortcut, another terminal directory, or a file manager.
std::string ResolveAssetPath(const std::string& requestedPath) {
    if (requestedPath.empty() || IsAbsoluteAssetPath(requestedPath)) return requestedPath;

    std::string appDirectory = GetApplicationDirectory();
    if (!appDirectory.empty() && appDirectory.back() != '/' && appDirectory.back() != '\\')
        appDirectory.push_back('/');

    const std::string executableRelativePath = appDirectory + requestedPath;
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    // Android's packaged asset paths can be virtual; preserve raylib's normal
    // relative lookup before trying the application directory.
    if (FileExists(requestedPath.c_str())) return requestedPath;
    if (FileExists(executableRelativePath.c_str())) return executableRelativePath;
#else
    if (FileExists(executableRelativePath.c_str())) return executableRelativePath;
    if (FileExists(requestedPath.c_str())) return requestedPath;
#endif
    // Return the preferred path even when absent so any diagnostic points at
    // the expected runtime location beside the executable.
    return executableRelativePath;
}

Model LoadModelFromPath(const std::string& requestedPath, std::string* loadedPath = nullptr) {
    if (loadedPath != nullptr) loadedPath->clear();
    const std::string path = ResolveAssetPath(requestedPath);
    if (!FileExists(path.c_str())) {
        TraceLog(LOG_DEBUG, "Optional 3D asset not found: %s", path.c_str());
        return Model{};
    }

    TraceLog(LOG_INFO, "Loading 3D model: %s", path.c_str());
    Model model = LoadModel(path.c_str());
    if (model.meshCount <= 0 || model.meshes == nullptr) {
        TraceLog(LOG_WARNING, "3D model failed to load or contains no meshes: %s", path.c_str());
        // UnloadModel iterates the mesh array, so only call it when that
        // array exists (malformed files can return an empty model).
        if (model.meshes != nullptr) UnloadModel(model);
        return Model{};
    }

    TraceLog(LOG_INFO, "Loaded 3D model (%i meshes): %s", model.meshCount, path.c_str());
    if (loadedPath != nullptr) *loadedPath = path;
    return model;
}

Model LoadModelFromManifest(const char* manifestPath, std::string* loadedPath = nullptr) {
    if (loadedPath != nullptr) loadedPath->clear();
    const std::string resolvedManifest = ResolveAssetPath(manifestPath);
    if (!FileExists(resolvedManifest.c_str())) return Model{};
    char* raw = LoadFileText(resolvedManifest.c_str());
    if (raw == nullptr) {
        TraceLog(LOG_WARNING, "Could not read 3D asset manifest: %s", resolvedManifest.c_str());
        return Model{};
    }

    std::string path(raw);
    UnloadFileText(raw);
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r' || path.back() == ' ' || path.back() == '\t')) path.pop_back();
    while (!path.empty() && (path.front() == ' ' || path.front() == '\t')) path.erase(path.begin());
    if (path.empty()) {
        TraceLog(LOG_WARNING, "Empty 3D asset manifest: %s", resolvedManifest.c_str());
        return Model{};
    }
    return LoadModelFromPath(path, loadedPath);
}

Model LoadOptionalModelStem(const char* stem, std::string* loadedPath = nullptr) {
    if (loadedPath != nullptr) loadedPath->clear();
    const char* extensions[] = { ".glb", ".gltf" };
    for (const char* ext : extensions) {
        const std::string requestedPath = std::string(stem) + ext;
        const std::string path = ResolveAssetPath(requestedPath);
        if (!FileExists(path.c_str())) continue;
        Model model = LoadModelFromPath(path, loadedPath);
        if (model.meshCount > 0) return model;
    }
    return Model{};
}

std::string Lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

int FindCharacterAnimation(const CharacterAnimationSet& set, const char* keyword) {
    if (set.clips == nullptr || set.count == 0 || keyword == nullptr) return -1;
    const std::string target = Lowercase(keyword);
    for (int i = 0; i < set.count; ++i) {
        if (Lowercase(set.clips[i].name) == target) return static_cast<int>(i);
    }
    for (int i = 0; i < set.count; ++i) {
        if (Lowercase(set.clips[i].name).find(target) != std::string::npos)
            return static_cast<int>(i);
    }
    return -1;
}

void UnloadCharacterAnimations(CharacterAnimationSet& set) {
    if (set.clips != nullptr) UnloadModelAnimations(set.clips, set.count);
    set = CharacterAnimationSet{};
}

void LoadCharacterAnimations(const std::string& modelPath, CharacterAnimationSet& set, const char* slot) {
    UnloadCharacterAnimations(set);
    if (modelPath.empty()) return;
    const std::string path = ResolveAssetPath(modelPath);
    if (!FileExists(path.c_str())) {
        TraceLog(LOG_WARNING, "%s animation source not found: %s", slot, path.c_str());
        return;
    }

    set.clips = LoadModelAnimations(path.c_str(), &set.count);
    if (set.clips == nullptr || set.count == 0) {
        TraceLog(LOG_WARNING, "%s model has no readable animation clips: %s", slot, path.c_str());
        if (set.clips != nullptr) UnloadModelAnimations(set.clips, set.count);
        set = CharacterAnimationSet{};
        return;
    }
    set.frameClocks.assign(set.count, 0.0f);
    set.idle = FindCharacterAnimation(set, "idle");
    set.walk = FindCharacterAnimation(set, "walk");
    set.run = FindCharacterAnimation(set, "run");
    set.attack = FindCharacterAnimation(set, "attack");
    set.hit = FindCharacterAnimation(set, "hit");
    set.death = FindCharacterAnimation(set, "death");
    if (set.idle < 0) set.idle = 0;
    if (set.walk < 0) set.walk = set.run >= 0 ? set.run : set.idle;
    if (set.run < 0) set.run = set.walk;

    TraceLog(LOG_INFO, "%s clips: %d (idle=%d, walk=%d, run=%d, attack=%d, hit=%d)",
             slot, set.count, set.idle, set.walk, set.run, set.attack, set.hit);
    for (int i = 0; i < set.count; ++i)
        TraceLog(LOG_DEBUG, "%s clip %d: '%s' (%d keyframes)", slot, i, set.clips[i].name, set.clips[i].keyframeCount);
}

int ChooseCharacterAnimation(const CharacterAnimationSet& set, bool moving, bool running,
                             bool attacking = false, bool hurt = false) {
    if (hurt && set.hit >= 0) return set.hit;
    if (attacking && set.attack >= 0) return set.attack;
    if (running && set.run >= 0) return set.run;
    if (moving && set.walk >= 0) return set.walk;
    if (set.idle >= 0) return set.idle;
    if (set.walk >= 0) return set.walk;
    return set.count > 0 ? 0 : -1;
}

void ApplyCharacterAnimation(Model model, CharacterAnimationSet& set, int requested, float dt,
                             bool restartOneShot = true) {
    if (set.clips == nullptr || set.count == 0) return;
    int selected = requested;
    if (selected < 0 || static_cast<unsigned int>(selected) >= set.count)
        selected = set.idle >= 0 ? set.idle : 0;
    const ModelAnimation& clip = set.clips[selected];
    if (model.skeleton.boneCount == 0 || clip.boneCount == 0 || model.skeleton.boneCount != clip.boneCount || clip.keyframeCount <= 0)
        return;
    if (set.frameClocks.size() != set.count) set.frameClocks.assign(set.count, 0.0f);
    const bool oneShot = selected == set.attack || selected == set.hit || selected == set.death;
    if (restartOneShot && oneShot && set.lastSelected != selected)
        set.frameClocks[static_cast<size_t>(selected)] = 0.0f;
    set.lastSelected = selected;
    float& clock = set.frameClocks[static_cast<size_t>(selected)];
    clock += std::max(0.0f, dt) * 30.0f;
    const int elapsed = static_cast<int>(clock);
    const int frame = oneShot ? std::min(elapsed, clip.keyframeCount - 1) : elapsed % clip.keyframeCount;
    UpdateModelAnimation(model, clip, frame);
}

float ComputeModelGroundOffset(Model model) {
    if (model.meshCount <= 0 || model.meshes == nullptr) return 0.0f;
    const BoundingBox bounds = GetModelBoundingBox(model);
    if (!std::isfinite(bounds.min.y) || !std::isfinite(bounds.max.y)) return 0.0f;
    const float height = bounds.max.y - bounds.min.y;
    if (height < 0.05f || height > 100.0f) return 0.0f;
    const float offset = -bounds.min.y;
    return std::isfinite(offset) && std::fabs(offset) < 100.0f ? offset : 0.0f;
}

void LoadOptionalNatureModels() {
    // User-installed Sketchfab GLBs replace the procedural tree/rock fallback geometry.
    natureTree = LoadOptionalModelStem("Assets/3d/sketchfab_tree");
    if (natureTree.meshCount == 0) natureTree = LoadModelFromManifest("Assets/3d/sketchfab_tree_path.txt");
    treeModelReady = natureTree.meshCount > 0;

    natureRock = LoadOptionalModelStem("Assets/3d/sketchfab_rock");
    if (natureRock.meshCount == 0) natureRock = LoadModelFromManifest("Assets/3d/sketchfab_rock_path.txt");
    rockModelReady = natureRock.meshCount > 0;

    // Optional user-installed Sketchfab models override the procedural character fallback.
    std::string heroModelPath;
    sketchfabHeroModel = LoadOptionalModelStem("Assets/3d/sketchfab_hero", &heroModelPath);
    if (sketchfabHeroModel.meshCount == 0)
        sketchfabHeroModel = LoadModelFromManifest("Assets/3d/sketchfab_hero_path.txt", &heroModelPath);
    heroModelFacesPositiveZ = false;
    sketchfabHeroReady = sketchfabHeroModel.meshCount > 0;

    std::string monsterModelPath;
    sketchfabMonsterModel = LoadOptionalModelStem("Assets/3d/sketchfab_monster", &monsterModelPath);
    if (sketchfabMonsterModel.meshCount == 0)
        sketchfabMonsterModel = LoadModelFromManifest("Assets/3d/sketchfab_monster_path.txt", &monsterModelPath);
    monsterModelFacesPositiveZ = false;
    sketchfabMonsterReady = sketchfabMonsterModel.meshCount > 0;

    std::string bossModelPath;
    sketchfabBossModel = LoadOptionalModelStem("Assets/3d/sketchfab_boss", &bossModelPath);
    if (sketchfabBossModel.meshCount == 0)
        sketchfabBossModel = LoadModelFromManifest("Assets/3d/sketchfab_boss_path.txt", &bossModelPath);
    bossModelFacesPositiveZ = false;
    sketchfabBossReady = sketchfabBossModel.meshCount > 0;

    if (sketchfabHeroReady) {
        heroGroundOffset = ComputeModelGroundOffset(sketchfabHeroModel);
        LoadCharacterAnimations(heroModelPath, heroAnimations, "hero");
    }
    if (sketchfabMonsterReady) {
        monsterGroundOffset = ComputeModelGroundOffset(sketchfabMonsterModel);
        LoadCharacterAnimations(monsterModelPath, monsterAnimations, "monster");
    }
    if (sketchfabBossReady) {
        bossGroundOffset = ComputeModelGroundOffset(sketchfabBossModel);
        LoadCharacterAnimations(bossModelPath, bossAnimations, "boss");
    }

    TraceLog(LOG_INFO,
        "3D asset status: tree=%s, rock=%s, hero=%s, monster=%s, boss=%s",
        treeModelReady ? "external model" : "procedural fallback",
        rockModelReady ? "external model" : "procedural fallback",
        sketchfabHeroReady ? "external model" : "procedural fallback",
        sketchfabMonsterReady ? "external model" : "procedural fallback",
        sketchfabBossReady ? "external model" : "procedural fallback");
}

void UnloadOptionalNatureModels() {
    if (treeModelReady) UnloadModel(natureTree);
    if (rockModelReady) UnloadModel(natureRock);
    UnloadCharacterAnimations(heroAnimations);
    UnloadCharacterAnimations(monsterAnimations);
    UnloadCharacterAnimations(bossAnimations);
    if (sketchfabHeroReady) UnloadModel(sketchfabHeroModel);
    if (sketchfabMonsterReady) UnloadModel(sketchfabMonsterModel);
    if (sketchfabBossReady) UnloadModel(sketchfabBossModel);
    treeModelReady = false;
    rockModelReady = false;
    sketchfabHeroReady = false;
    sketchfabMonsterReady = false;
    sketchfabBossReady = false;
    heroGroundOffset = 0.0f;
    monsterGroundOffset = 0.0f;
    bossGroundOffset = 0.0f;
}

void InitCamera() {
    const Vector3 focus = V3(player.position.x, player.position.y + 1.0f, player.position.z);
    const float horizontal = cameraDistance * std::cos(cameraPitch);
    camera.position = V3(focus.x + std::sin(cameraYaw) * horizontal,
                         focus.y + std::sin(cameraPitch) * cameraDistance,
                         focus.z + std::cos(cameraYaw) * horizontal);
    camera.target = focus;
    camera.up = V3(0.0f, 1.0f, 0.0f);
    camera.fovy = 52.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void DrawTree(const Prop& p) {
    const float s = p.scale;
    const float ground = p.position.y;
    if (treeModelReady) {
        DrawModel(natureTree, V3(p.position.x, ground, p.position.z), 4.0f*s, WHITE);
        return;
    }
    const Biome b = GetBiome(p.position.x);
    const Color leaves = b == Biome::Ember ? Color{ 116, 69, 52, 255 }
                       : (b == Biome::Frost ? Color{ 98, 137, 145, 255 }
                       : (b == Biome::Dune ? Color{ 143, 126, 87, 255 } : Color{ 50, 86, 48, 255 }));
    const float trunkH = (p.kind == 3 ? 2.1f : 3.0f)*s;
    DrawCylinder(V3(p.position.x, ground+trunkH*0.5f, p.position.z), 0.19f*s, 0.29f*s, trunkH, 9,
                 p.kind == 3 ? Color{ 74, 57, 48, 255 } : Color{ 83, 65, 45, 255 });
    if (p.kind == 3) {
        DrawCylinderEx(V3(p.position.x, ground+trunkH, p.position.z),
                       V3(p.position.x+0.9f*s, ground+trunkH+0.65f*s, p.position.z+0.15f*s),
                       0.13f*s, 0.035f*s, 5, Color{ 86, 68, 56, 255 });
        DrawCylinderEx(V3(p.position.x, ground+trunkH*0.75f, p.position.z),
                       V3(p.position.x-0.8f*s, ground+trunkH+0.45f*s, p.position.z-0.2f*s),
                       0.12f*s, 0.025f*s, 5, Color{ 86, 68, 56, 255 });
        return;
    }
    // Tapered, layered tree crowns replace the old oversized sphere clusters.
    DrawCylinder(V3(p.position.x, ground+trunkH+0.34f*s, p.position.z),
                 0.025f*s, 0.78f*s, 1.45f*s, 9, leaves);
    DrawCylinder(V3(p.position.x, ground+trunkH+1.08f*s, p.position.z),
                 0.015f*s, 0.60f*s, 1.35f*s, 9, BlendColor(leaves, WHITE, 0.06f));
    DrawCylinder(V3(p.position.x, ground+trunkH+1.68f*s, p.position.z),
                 0.0f, 0.40f*s, 1.05f*s, 9, leaves);
    DrawSphere(V3(p.position.x, ground+trunkH+0.22f*s, p.position.z), 0.23f*s, BlendColor(leaves, WHITE, 0.035f));
}
void DrawRock(const Prop& p) {
    const float s = p.scale;
    if (rockModelReady) {
        DrawModel(natureRock, V3(p.position.x, p.position.y, p.position.z), 4.2f*s, WHITE);
        return;
    }
    const Color col = GetBiome(p.position.x) == Biome::Frost ? Color{ 133, 151, 151, 255 }
                    : (GetBiome(p.position.x) == Biome::Ember ? Color{ 91, 70, 65, 255 }
                    : Color{ 105, 108, 96, 255 });
    if (fallbackRockReady) {
        const Vector3 scale = V3(1.15f*s, 0.46f*s, 0.88f*s);
        DrawModelEx(fallbackRockModel, V3(p.position.x, p.position.y+0.23f*s, p.position.z),
                    V3(0,1,0), static_cast<float>(p.variant*37), scale, col);
    } else {
        DrawSphere(V3(p.position.x, p.position.y+0.22f*s, p.position.z), 0.5f*s, col);
    }
}
void DrawEnvironment() {
    if (terrainReady) DrawModel(terrainModel, V3(0.0f, 0.0f, 0.0f), 1.0f, WHITE);
    else {
        // Safe fallback if texture/mesh allocation fails at startup.
        for (int ix = -3; ix < 16; ++ix) {
            for (int iz = -4; iz < 4; ++iz) {
                const float x = ix*16.0f + 8.0f;
                const float z = iz*16.0f + 8.0f;
                DrawPlane(V3(x, 0.0f, z), Vector2{16.0f, 16.0f}, GroundColor(GetBiome(x), 1));
            }
        }
    }
    // The broad dirt travel trail is painted into the terrain's 1536x512 albedo map.
    for (const Prop& p : props) {
        if (DistXZ(p.position, player.position) > 84.0f) continue;
        switch (p.kind) {
            case 0: case 3: DrawTree(p); break;
            case 1: DrawRock(p); break;
            case 2:
                DrawCylinder(V3(p.position.x, p.position.y+0.8f*p.scale, p.position.z), 0.14f*p.scale, 0.36f*p.scale,
                             1.6f*p.scale, 5, GetBiome(p.position.x)==Biome::Frost ? C_CYAN : C_VIOLET);
                DrawSphere(V3(p.position.x, p.position.y+1.75f*p.scale, p.position.z), 0.28f*p.scale, C_CYAN);
                break;
            default:
                DrawSphere(V3(p.position.x, p.position.y+0.32f*p.scale, p.position.z), 0.52f*p.scale,
                           GetBiome(p.position.x)==Biome::Ember ? Color{ 148, 69, 54, 255 } : Color{ 47, 127, 73, 255 });
                break;
        }
    }
    // Quest altars and distant landmarks.
    for (int i=0; i<BOSS_COUNT; ++i) {
        const Vector3 b = bosses[i].position;
        const bool unlocked = (i==0 ? player.kills >= 3 : !bosses[i-1].alive);
        const Color marker = unlocked ? BossColor(i) : Color{ 110, 119, 137, 255 };
        DrawCylinder(V3(b.x, b.y+0.17f, b.z), 1.25f, 1.25f, 0.34f, 12, Color{ 35, 40, 55, 255 });
        DrawCylinder(V3(b.x, b.y+0.34f, b.z), 0.83f, 0.83f, 0.18f, 12, marker);
        DrawCylinderEx(V3(b.x, b.y+0.8f, b.z), V3(b.x, b.y+3.3f + std::sin(worldClock*2.0f+i)*0.25f, b.z),
                       0.06f, 0.015f, 7, WithAlpha(marker, 160));
        DrawSphere(V3(b.x, b.y+3.45f + std::sin(worldClock*2.0f+i)*0.25f, b.z), 0.2f, marker);
    }
}

void DrawPlayerModel() {
    const Vector3 p = player.position;
    DrawCircle3D(V3(p.x, TerrainHeightAt(p.x, p.z)+0.018f, p.z), 0.62f, V3(1.0f, 0.0f, 0.0f), 90.0f,
                 WithAlpha(Color{ 14, 22, 25, 255 }, 105));
    if (sketchfabHeroReady) {
        const int heroAnimation = ChooseCharacterAnimation(
            heroAnimations, player.moving, player.running,
            player.attackClock > 0.0f, player.hurtFlash > 0.0f);
        ApplyCharacterAnimation(sketchfabHeroModel, heroAnimations, heroAnimation, GetFrameTime());
        // Applying the GLB's idle pose also keeps skinned characters upright/visible.
        // Sketchfab models conventionally face -Z; bundled Gobkit CC0 models face +Z.
        const float yaw = heroModelFacesPositiveZ
            ? std::atan2(player.facing.x, player.facing.z) * RAD2DEG
            : std::atan2(-player.facing.x, -player.facing.z) * RAD2DEG;
        const float modelScale = 1.35f;
        DrawModelEx(sketchfabHeroModel,
                    V3(p.x, p.y + heroGroundOffset*modelScale, p.z),
                    V3(0.0f, 1.0f, 0.0f), yaw,
                    V3(modelScale, modelScale, modelScale), WHITE);
    } else {
        const float step = std::sin(player.walkClock*10.5f) * 0.23f;
        const float bodyBob = player.grounded ? std::fabs(std::sin(player.walkClock*10.5f))*0.05f : 0.0f;
        Color armor = player.hurtFlash > 0.0f ? WHITE : Color{ 65, 109, 164, 255 };
        Color cloth = Color{ 32, 54, 89, 255 };
        DrawCube(V3(p.x, p.y+1.10f+bodyBob, p.z), 0.56f, 0.78f, 0.36f, armor);
        DrawCube(V3(p.x, p.y+0.64f+bodyBob, p.z-0.02f), 0.62f, 0.32f, 0.41f, cloth);
        DrawCube(V3(p.x, p.y+1.08f+bodyBob, p.z+0.235f), 0.38f, 0.60f, 0.09f, Color{ 112, 47, 59, 255 });
        DrawSphere(V3(p.x-0.31f, p.y+1.38f+bodyBob, p.z), 0.19f, Color{ 104, 143, 190, 255 });
        DrawSphere(V3(p.x+0.31f, p.y+1.38f+bodyBob, p.z), 0.19f, Color{ 104, 143, 190, 255 });
        DrawSphere(V3(p.x, p.y+1.14f+bodyBob, p.z-0.205f), 0.09f, C_GOLD);
        DrawSphere(V3(p.x, p.y+1.75f+bodyBob, p.z), 0.25f, Color{ 226, 188, 151, 255 });
        DrawCylinder(V3(p.x, p.y+2.00f+bodyBob, p.z), 0.15f, 0.22f, 0.12f, 7, C_GOLD);
        DrawCube(V3(p.x, p.y+1.78f+bodyBob, p.z-0.215f), 0.26f, 0.075f, 0.045f, C_CYAN);
        const Vector3 leftHip = V3(p.x-0.18f, p.y+0.58f, p.z);
        const Vector3 rightHip = V3(p.x+0.18f, p.y+0.58f, p.z);
        DrawCylinderEx(leftHip, V3(p.x-0.18f+step, p.y+0.10f, p.z), 0.105f, 0.08f, 6, Color{ 49, 65, 83, 255 });
        DrawCylinderEx(rightHip, V3(p.x+0.18f-step, p.y+0.10f, p.z), 0.105f, 0.08f, 6, Color{ 49, 65, 83, 255 });
        DrawCylinderEx(V3(p.x-0.32f, p.y+1.36f, p.z), V3(p.x-0.42f, p.y+0.92f, p.z-0.04f), 0.095f, 0.07f, 6, armor);
        DrawCylinderEx(V3(p.x+0.32f, p.y+1.36f, p.z), V3(p.x+0.42f, p.y+0.92f, p.z-0.04f), 0.095f, 0.07f, 6, armor);
        const Vector3 hand = V3(p.x+0.42f, p.y+0.94f, p.z-0.06f);
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
    }
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
    if (sketchfabMonsterReady) {
        const float dx = player.position.x - p.x;
        const float dz = player.position.z - p.z;
        const float yaw = monsterModelFacesPositiveZ
            ? std::atan2(dx, dz) * RAD2DEG
            : std::atan2(-dx, -dz) * RAD2DEG;
        const float modelScale = scale*1.35f;
        DrawModelEx(sketchfabMonsterModel,
                    V3(p.x, p.y + monsterGroundOffset*modelScale, p.z),
                    V3(0.0f, 1.0f, 0.0f), yaw,
                    V3(modelScale, modelScale, modelScale), WHITE);
        return;
    }
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
    if (sketchfabBossReady) {
        const float dx = player.position.x - b.position.x;
        const float dz = player.position.z - b.position.z;
        const float yaw = bossModelFacesPositiveZ
            ? std::atan2(dx, dz) * RAD2DEG
            : std::atan2(-dx, -dz) * RAD2DEG;
        const float modelScale = 1.45f*scale*pulse;
        DrawModelEx(sketchfabBossModel,
                    V3(b.position.x, b.position.y + bossGroundOffset*modelScale, b.position.z),
                    V3(0.0f, 1.0f, 0.0f), yaw,
                    V3(modelScale, modelScale, modelScale), WHITE);
    } else {
        DrawMonsterModel(b.position, i, b.phase, scale*pulse, c);
    }
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
    // Advance shared model animation once per frame, then draw all instances
    // using the same skinned pose rather than restarting each clip per enemy.
    const float animationDt = GetFrameTime();
    const bool worldActive = screen == Screen::Playing;
    if (sketchfabMonsterReady) {
        const int animation = ChooseCharacterAnimation(monsterAnimations, worldActive, false);
        ApplyCharacterAnimation(sketchfabMonsterModel, monsterAnimations, animation, animationDt, false);
    }
    if (sketchfabBossReady) {
        bool anyBossAdvancing = false;
        for (int i = 0; i < BOSS_COUNT; ++i) {
            const bool unlocked = (i == 0) ? (player.kills >= 3) : !bosses[i - 1].alive;
            if (bosses[i].alive && unlocked && DistXZ(player.position, bosses[i].position) > 3.4f) {
                anyBossAdvancing = true;
                break;
            }
        }
        const int animation = ChooseCharacterAnimation(bossAnimations, worldActive && anyBossAdvancing, false);
        ApplyCharacterAnimation(sketchfabBossModel, bossAnimations, animation, animationDt, false);
    }
    for (const Monster& m : monsters) {
        if (!m.alive || DistXZ(player.position, m.position)>75.0f) continue;
        DrawCircle3D(V3(m.position.x, m.position.y+0.016f, m.position.z), 0.48f, V3(1.0f, 0.0f, 0.0f), 90.0f,
                     WithAlpha(Color{ 12, 18, 20, 255 }, 95));
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
    // Convert keyboard/touch input from camera space into world space.
    const float inputX = move.x;
    const float inputZ = move.z;
    const float sinYaw = std::sin(cameraYaw);
    const float cosYaw = std::cos(cameraYaw);
    move.x = inputX*cosYaw + inputZ*sinYaw;
    move.z = -inputX*sinYaw + inputZ*cosYaw;
    const float len = std::sqrt(move.x*move.x+move.z*move.z);
    if (len > 1.0f) { move.x/=len; move.z/=len; }
    player.moving = len > 0.1f;
#if !defined(__ANDROID__) && !defined(PLATFORM_ANDROID)
    const bool sprintKeyDown = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
#else
    const bool sprintKeyDown = false;
#endif
    player.running = player.moving && (sprintKeyDown || player.abilityClock > 0.0f);
    if (player.moving) {
        player.facing = NormalizeXZ(move);
        player.walkClock += dt;
    }
    const float speed = player.abilityClock > 0.0f ? 12.0f : (player.running ? 10.0f : 7.0f);
    player.position.x += move.x*speed*dt;
    player.position.z += move.z*speed*dt;
    player.position.x = ClampF(player.position.x, WORLD_MIN_X, WORLD_MAX_X);
    player.position.z = ClampF(player.position.z, -WORLD_HALF_Z+2.0f, WORLD_HALF_Z-2.0f);

#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    const Rectangle jumpButton = ButtonRect(sw-106*uiScale, sh-205*uiScale, 54*uiScale, 42*uiScale);
    const bool jumpPressed = IsKeyPressed(KEY_SPACE) || Pressed(jumpButton);
#else
    const bool jumpPressed = IsKeyPressed(KEY_SPACE);
#endif
    if (jumpPressed && player.grounded) {
        player.velocity.y = 8.3f;
        player.grounded = false;
        SpawnBurst(V3(player.position.x, 0.15f, player.position.z), C_GOLD, 8, 1.8f);
    }
    player.velocity.y -= 20.0f*dt;
    player.position.y += player.velocity.y*dt;
    const float groundY = TerrainHeightAt(player.position.x, player.position.z);
    if (player.position.y <= groundY) {
        if (!player.grounded && player.velocity.y < -5.0f)
            SpawnBurst(V3(player.position.x, groundY+0.12f, player.position.z), Color{ 190, 194, 161, 255 }, 8, 1.4f);
        player.position.y = groundY;
        player.velocity.y = 0.0f;
        player.grounded = true;
    }

#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    if (IsKeyPressed(KEY_J) || Pressed(ButtonRect(sw-104*uiScale, sh-104*uiScale, 90*uiScale, 90*uiScale))) PlayerAttack();
    if (IsKeyPressed(KEY_K) || Pressed(ButtonRect(sw-220*uiScale, sh-154*uiScale, 80*uiScale, 80*uiScale))) PlayerAbility();
#else
    // Right mouse is reserved for camera orbit, so Blink stays on K on desktop.
    if (IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) PlayerAttack();
    if (IsKeyPressed(KEY_K)) PlayerAbility();
#endif

    // Orbit with right-mouse drag and zoom with the wheel on desktop.
#if !defined(__ANDROID__) && !defined(PLATFORM_ANDROID)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        const Vector2 mouseDelta = GetMouseDelta();
        cameraYaw -= mouseDelta.x * 0.006f;
        cameraPitch = ClampF(cameraPitch + mouseDelta.y * 0.0045f, 0.16f, 0.92f);
    }
    cameraDistance = ClampF(cameraDistance - GetMouseWheelMove() * 1.25f, 6.5f, 18.0f);
#endif
    const Vector3 focus = V3(player.position.x, player.position.y + 1.0f, player.position.z);
    const float horizontal = cameraDistance * std::cos(cameraPitch);
    const Vector3 desiredPos = V3(focus.x + std::sin(cameraYaw) * horizontal,
                                  focus.y + std::sin(cameraPitch) * cameraDistance,
                                  focus.z + std::cos(cameraYaw) * horizontal);
    camera.position = Vector3Lerp(camera.position, desiredPos, ClampF(dt*7.0f, 0.0f, 1.0f));
    camera.target = Vector3Lerp(camera.target, focus, ClampF(dt*8.0f, 0.0f, 1.0f));

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
        m.position.y = TerrainHeightAt(m.position.x, m.position.z);
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
        b.position.y = TerrainHeightAt(b.position.x, b.position.z);
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
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
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

#else
    const char* desktopControls = "WASD: MOVE   SPACE: JUMP   LEFT CLICK / J: ATTACK   RIGHT-DRAG: CAMERA   WHEEL: ZOOM   K: BLINK";
    DrawRectangleRounded({16*scale,sh-62*scale,sw-32*scale,22*scale},0.2f,6,WithAlpha(C_INK,175));
    DrawText(desktopControls,static_cast<int>(25*scale),static_cast<int>(sh-56*scale),
             static_cast<int>(11*scale),Color{210,224,240,255});
#endif
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
    #if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    DrawText("TOUCH PAD: MOVE   JUMP   ATTACK   BLINK",
             static_cast<int>(sw*0.135f),static_cast<int>(sh*0.75f),static_cast<int>(12*scale),Color{210,224,240,255});
#else
    DrawText("WASD: MOVE   SPACE: JUMP   LEFT CLICK / J: ATTACK   RIGHT-DRAG: CAMERA   WHEEL: ZOOM   K: BLINK",
             static_cast<int>(sw*0.135f),static_cast<int>(sh*0.75f),static_cast<int>(12*scale),Color{210,224,240,255});
#endif
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
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(DESIGN_W, DESIGN_H, "Wildbound: Boss Hunter");
#else
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "Wildbound: Boss Hunter");
#endif
    SetTargetFPS(60);
    InitAudio();
    LoadProceduralTerrain();
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
                player.position.y = TerrainHeightAt(player.position.x, player.position.z);
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

        if (screen != Screen::Playing) {
            player.moving = false;
            player.running = false;
        }

        const Biome biome=GetBiome(player.position.x);
        const Color skyTop = biome==Biome::Dune ? Color{ 184, 103, 58, 255 }
                           : (biome==Biome::Frost ? Color{ 62, 112, 163, 255 }
                           : (biome==Biome::Ember ? Color{ 54, 42, 70, 255 } : Color{ 47, 104, 143, 255 }));
        const Color skyHorizon = biome==Biome::Dune ? Color{ 247, 216, 160, 255 }
                               : (biome==Biome::Frost ? Color{ 194, 222, 231, 255 }
                               : (biome==Biome::Ember ? Color{ 196, 102, 75, 255 } : Color{ 165, 205, 198, 255 }));
        BeginDrawing();
        ClearBackground(skyTop);
        DrawRectangleGradientV(0, 0, sw, sh, skyTop, skyHorizon);
        BeginMode3D(camera);
            DrawEnvironment();
            DrawWorldEntities();
            DrawPlayerModel();
            // Removed the diagnostic grid that popped in and out near the center lane.
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
    UnloadProceduralTerrain();
    CloseAudio();
    CloseWindow();
    return 0;
}

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int VIEW_W = 960;
constexpr int VIEW_H = 540;
float WORLD_W = 3550.0f;
constexpr int LEVEL_COUNT = 10;
constexpr float PLAYER_W = 36.0f;
constexpr float PLAYER_H = 48.0f;
constexpr float GRAVITY = 1120.0f;
constexpr float JUMP_SPEED = 485.0f;

const Color C_INK       = { 17, 25, 47, 255 };
const Color C_PANEL     = { 26, 39, 68, 245 };
const Color C_PANEL_LIT = { 39, 57, 91, 255 };
const Color C_MINT      = { 107, 245, 190, 255 };
const Color C_SKY_BLUE  = { 93, 190, 255, 255 };
const Color C_GOLD      = { 255, 211, 92, 255 };
const Color C_CORAL     = { 255, 112, 125, 255 };
const Color C_WHITE_90  = { 242, 247, 255, 235 };

enum class Screen { Menu, Controls, Playing, Paused, GameOver, Victory };

struct Platform {
    Rectangle box;
    bool moving = false;
    float originX = 0.0f;
    float travel = 0.0f;
    float moveSpeed = 0.0f;
    float phase = 0.0f;
};
struct Spike { Rectangle box; int type = 0; };
struct Coin { Vector2 center; bool taken = false; float phase = 0.0f; };
struct Enemy {
    Rectangle box;
    float minX = 0.0f;
    float maxX = 0.0f;
    float speed = 75.0f;
    int direction = 1;
    bool alive = true;
};
struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float radius;
    Color color;
};
struct Player {
    Vector2 pos { 70.0f, 422.0f };
    Vector2 vel { 0.0f, 0.0f };
    bool grounded = false;
    int lives = 3;
};

Screen screen = Screen::Menu;
RenderTexture2D canvas {};
Texture2D characterAtlas {};
Texture2D backgroundTexture {};
Image backgroundFrames {};
int backgroundFrameCount = 0;
int currentBackgroundFrame = 0;
float backgroundFrameTimer = 0.0f;
float landingTimer = 0.0f;
float runDustTimer = 0.0f;
float playerFacing = 1.0f;
Vector2 pointer { 0.0f, 0.0f };
bool touchDown = false;
bool touchPressed = false;
bool previousJumpTouch = false;
int previousTouchCount = 0;
float viewScale = 1.0f;
float viewOffsetX = 0.0f;
float viewOffsetY = 0.0f;

std::vector<Platform> platforms;
std::vector<Spike> spikes;
std::vector<Coin> coins;
std::vector<Enemy> enemies;
std::vector<Particle> particles;

Sound sfxJump {};
Sound sfxCoin {};
Sound sfxHurt {};
Sound sfxStomp {};
Sound sfxLevel {};
Sound sfxMenu {};
bool audioReady = false;
Font uiFont {};
bool customUiFont = false;

Player player;
int levelIndex = 0;
int difficulty = 0; // 0 Easy, 1 Normal, 2 Hard
int score = 0;
int highScore = 0;
float cameraX = 0.0f;
float gameClock = 0.0f;
float hurtFlash = 0.0f;
float damageNoticeTimer = 0.0f;
float scoreNoticeTimer = 0.0f;
int scoreNoticeValue = 0;
Vector2 scoreNoticePos { 0.0f, 0.0f };
float respawnTimer = 0.0f;
float screenShakeTimer = 0.0f;
float impactTimer = 0.0f;
Vector2 impactScreenPos { VIEW_W * 0.5f, VIEW_H * 0.5f };
bool pendingGameOver = false;
bool exitRequested = false;

const char* DifficultyName() {
    if (difficulty == 0) return "EASY";
    if (difficulty == 1) return "MEDIUM";
    return "HARD";
}

const char* LevelName(int level) {
    static const char* names[LEVEL_COUNT] = {
        "WATERFALL TRAIL", "RIVER CROSSING", "ANCIENT RUINS",
        "MISTY WOODS", "CRYSTAL CAVES", "SANDSTONE RISE",
        "FROSTLINE RIDGE", "EMBER PATH", "MOONLIT BRIDGES",
        "THE FINAL ASCENT"
    };
    return names[std::clamp(level, 0, LEVEL_COUNT - 1)];
}

int StartingLives() {
    if (difficulty == 0) return 5;
    if (difficulty == 1) return 3;
    return 2;
}

Vector2 ToVirtual(Vector2 p) {
    return { (p.x - viewOffsetX) / viewScale, (p.y - viewOffsetY) / viewScale };
}

void UpdatePointer() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    viewScale = std::min(sw / static_cast<float>(VIEW_W),
                         sh / static_cast<float>(VIEW_H));
    viewOffsetX = (sw - VIEW_W * viewScale) * 0.5f;
    viewOffsetY = (sh - VIEW_H * viewScale) * 0.5f;

    touchDown = GetTouchPointCount() > 0;
    const Vector2 raw = touchDown ? GetTouchPosition(0) : GetMousePosition();
    pointer = ToVirtual(raw);
    touchPressed = touchDown && previousTouchCount == 0;
    previousTouchCount = GetTouchPointCount();
}

bool PointerInside(Rectangle r) {
    return CheckCollisionPointRec(pointer, r);
}

bool Pressed(Rectangle r) {
    return (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && PointerInside(r)) ||
           (touchPressed && PointerInside(r));
}

bool TouchHeld(Rectangle r) {
    const int count = GetTouchPointCount();
    for (int i = 0; i < count; ++i) {
        if (CheckCollisionPointRec(ToVirtual(GetTouchPosition(i)), r)) return true;
    }
    return false;
}

bool Held(Rectangle r) {
    return (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && PointerInside(r)) ||
           TouchHeld(r);
}

bool Button(Rectangle r, const char* label, bool active = true) {
    const bool hover = PointerInside(r);
    const Color fill = !active ? Color{ 39, 46, 64, 255 }
                              : (hover ? C_PANEL_LIT : C_PANEL);
    DrawRectangleRounded(r, 0.22f, 8, fill);
    DrawRectangleRoundedLinesEx(r, 0.22f, 8, 2.0f,
                              active && hover ? C_MINT : Color{ 76, 96, 133, 255 });
    const int fs = 19;
    const int tw = MeasureUIText(label, fs);
    DrawUIText(label, static_cast<int>(r.x + (r.width - tw) * 0.5f),
             static_cast<int>(r.y + (r.height - fs) * 0.5f - 1),
             fs, active ? C_WHITE_90 : Color{ 133, 145, 165, 255 });
    return active && Pressed(r);
}

void TextCentered(const char* text, int y, int size, Color color) {
    DrawUIText(text, (VIEW_W - MeasureUIText(text, size)) / 2, y, size, color);
}

Sound MakeTone(float frequency, float duration, float volume, bool noisy = false) {
    constexpr unsigned int sampleRate = 22050;
    const unsigned int frames = static_cast<unsigned int>(sampleRate * duration);
    short* samples = static_cast<short*>(MemAlloc(frames * sizeof(short)));
    if (samples == nullptr) return Sound{};

    for (unsigned int i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float progress = t / duration;
        const float envelope = std::min(1.0f, t / 0.012f) *
                               std::max(0.0f, (duration - t) / 0.035f);
        const float phase = 2.0f * PI * frequency * t *
                            (1.0f - 0.16f * progress);
        float value = std::sin(phase);
        if (noisy) {
            unsigned int seed = i * 1103515245u + 12345u;
            const float noise = (static_cast<float>((seed >> 16) & 0x7fff) / 16383.5f) - 1.0f;
            value = value * 0.72f + noise * 0.28f;
        }
        const float sample = std::clamp(value * envelope * volume, -1.0f, 1.0f);
        samples[i] = static_cast<short>(sample * 32767.0f);
    }

    Wave wave = { frames, sampleRate, 16, 1, samples };
    Sound sound = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return sound;
}

void PlayFx(Sound sound) {
    if (audioReady && sound.frameCount > 0) PlaySound(sound);
}

void InitGameAudio() {
    InitAudioDevice();
    audioReady = IsAudioDeviceReady();
    if (!audioReady) return;

    SetMasterVolume(0.78f);
    sfxJump = MakeTone(540.0f, 0.12f, 0.40f);
    sfxCoin = MakeTone(980.0f, 0.14f, 0.32f);
    sfxHurt = MakeTone(150.0f, 0.30f, 0.52f, true);
    sfxStomp = MakeTone(250.0f, 0.10f, 0.35f);
    sfxLevel = MakeTone(680.0f, 0.24f, 0.28f);
    sfxMenu = MakeTone(420.0f, 0.07f, 0.20f);
}

void CloseGameAudio() {
    if (!audioReady) return;
    UnloadSound(sfxJump);
    UnloadSound(sfxCoin);
    UnloadSound(sfxHurt);
    UnloadSound(sfxStomp);
    UnloadSound(sfxLevel);
    UnloadSound(sfxMenu);
    CloseAudioDevice();
    audioReady = false;
}


void LoadReadableFont() {
    uiFont = GetFontDefault();
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    const char* paths[] = {
        "assets/Roboto-Regular.ttf",
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/Roboto-Medium.ttf"
    };
#elif defined(_WIN32)
    const char* paths[] = {
        "assets/Roboto-Regular.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };
#else
    const char* paths[] = {
        "assets/Roboto-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
    };
#endif
    for (const char* path : paths) {
        if (!FileExists(path)) continue;
        Font candidate = LoadFontEx(path, 64, nullptr, 0);
        if (candidate.texture.id != 0) {
            uiFont = candidate;
            customUiFont = true;
            SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
            TraceLog(LOG_INFO, "UI: Loaded readable font from %s", path);
            return;
        }
    }
    TraceLog(LOG_WARNING, "UI: No system TTF font found; using raylib default font");
}

void UnloadReadableFont() {
    if (customUiFont) UnloadFont(uiFont);
    uiFont = {};
    customUiFont = false;
}

void DrawUIText(const char* text, int x, int y, int size, Color color) {
    if (text == nullptr) return;
    if (uiFont.texture.id == 0) uiFont = GetFontDefault();
    DrawTextEx(uiFont, text, { static_cast<float>(x), static_cast<float>(y) },
               static_cast<float>(size),
               std::max(0.5f, static_cast<float>(size) * 0.015f), color);
}

int MeasureUIText(const char* text, int size) {
    if (text == nullptr) return 0;
    if (uiFont.texture.id == 0) uiFont = GetFontDefault();
    const Vector2 measured = MeasureTextEx(
        uiFont, text, static_cast<float>(size),
        std::max(0.5f, static_cast<float>(size) * 0.015f));
    return static_cast<int>(std::ceil(measured.x));
}

void SpawnParticles(Vector2 pos, Color color, int amount = 16) {
    for (int i = 0; i < amount; ++i) {
        const float a = (static_cast<float>(GetRandomValue(0, 359)) * PI) / 180.0f;
        const float speed = static_cast<float>(GetRandomValue(55, 220));
        particles.push_back({ pos, { std::cos(a) * speed, std::sin(a) * speed },
                              GetRandomValue(35, 85) / 100.0f,
                              0.85f, static_cast<float>(GetRandomValue(2, 5)), color });
    }
}

void SpawnFootDust(Vector2 pos, int amount = 5) {
    for (int i = 0; i < amount; ++i) {
        const float vx = static_cast<float>(GetRandomValue(-85, 85));
        const float vy = static_cast<float>(GetRandomValue(-95, -20));
        const float life = GetRandomValue(22, 42) / 100.0f;
        const Color dust = (i % 2 == 0)
            ? Color{ 218, 239, 221, 220 }
            : Color{ 171, 211, 199, 210 };
        particles.push_back({ pos, { vx, vy }, life, life,
                              static_cast<float>(GetRandomValue(2, 4)), dust });
    }
}

const char* BackgroundAssetPath() {
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    return "background.gif";
#else
    if (FileExists("assets/background.gif")) return "assets/background.gif";
    if (FileExists("../assets/background.gif")) return "../assets/background.gif";
    return "assets/background.gif";
#endif
}

const char* CharacterAssetPath() {
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    return "character_atlas.png";
#else
    if (FileExists("assets/character_atlas.png")) return "assets/character_atlas.png";
    if (FileExists("../assets/character_atlas.png")) return "../assets/character_atlas.png";
    return "assets/character_atlas.png";
#endif
}

void LoadGameAssets() {
    backgroundFrames = LoadImageAnim(BackgroundAssetPath(), &backgroundFrameCount);
    if (backgroundFrames.data != nullptr && backgroundFrameCount > 0) {
        const Image firstFrame = {
            backgroundFrames.data,
            backgroundFrames.width,
            backgroundFrames.height,
            backgroundFrames.mipmaps,
            backgroundFrames.format
        };
        backgroundTexture = LoadTextureFromImage(firstFrame);
        if (backgroundTexture.id != 0) SetTextureFilter(backgroundTexture, TEXTURE_FILTER_BILINEAR);
    } else {
        TraceLog(LOG_WARNING, "ASSET: Could not load animated background GIF; using fallback background");
        backgroundFrameCount = 0;
    }

    characterAtlas = LoadTexture(CharacterAssetPath());
    if (characterAtlas.id != 0) SetTextureFilter(characterAtlas, TEXTURE_FILTER_BILINEAR);
    else TraceLog(LOG_WARNING, "ASSET: Could not load character_atlas.png; using fallback character");
}

void UpdateBackgroundAnimation(float dt) {
    if (backgroundFrames.data == nullptr || backgroundTexture.id == 0 ||
        backgroundFrameCount < 2) return;

    backgroundFrameTimer += dt;
    constexpr float frameDuration = 0.10f;
    if (backgroundFrameTimer < frameDuration) return;

    while (backgroundFrameTimer >= frameDuration) {
        backgroundFrameTimer -= frameDuration;
        currentBackgroundFrame = (currentBackgroundFrame + 1) % backgroundFrameCount;
    }

    const size_t frameBytes = static_cast<size_t>(backgroundFrames.width) *
                              static_cast<size_t>(backgroundFrames.height) * 4u;
    const auto* framePixels = static_cast<const unsigned char*>(backgroundFrames.data) +
                              frameBytes * static_cast<size_t>(currentBackgroundFrame);
    UpdateTexture(backgroundTexture, framePixels);
}

void UnloadGameAssets() {
    if (backgroundTexture.id != 0) UnloadTexture(backgroundTexture);
    if (characterAtlas.id != 0) UnloadTexture(characterAtlas);
    if (backgroundFrames.data != nullptr) UnloadImage(backgroundFrames);
    backgroundTexture = {};
    characterAtlas = {};
    backgroundFrames = {};
}

void AddCoin(float x, float y, float phase) {
    coins.push_back({ { x, y }, false, phase });
}

void AddEnemy(float x, float y, float minX, float maxX, float baseSpeed) {
    const float modeScale = difficulty == 0 ? 0.78f : (difficulty == 2 ? 1.30f : 1.0f);
    const float progressionScale = 1.0f + static_cast<float>(levelIndex) * 0.035f;
    // Keep enemy feet aligned to visible platform tops.
    enemies.push_back({ { x, y - 5.0f, 36.0f, 32.0f }, minX, maxX,
                        baseSpeed * modeScale * progressionScale, 1, true });
}

void LoadLevel(int which) {
    levelIndex = std::clamp(which, 0, LEVEL_COUNT - 1);
    // Ten longer routes; every stage grows slightly and has its own visual theme/layout.
    WORLD_W = 3550.0f + static_cast<float>(levelIndex) * 180.0f;
    platforms.clear();
    spikes.clear();
    coins.clear();
    enemies.clear();
    particles.clear();

    if (levelIndex == 0) {
        platforms = {
            { { 0, 470, 470, 100 } }, { { 520, 430, 180, 30 } },
            { { 760, 385, 150, 30 } }, { { 970, 450, 400, 100 } },
            { { 1410, 405, 170, 30 } }, { { 1640, 360, 160, 30 } },
            { { 1840, 440, 240, 100 } }, { { 2110, 395, 180, 30 } },
            { { 2350, 450, 490, 100 } }
        };
        spikes = { { { 248, 444, 52, 26 }, 0 }, { { 1080, 424, 54, 26 }, 0 },
                   { { 1915, 414, 50, 26 }, 0 }, { { 2490, 424, 50, 26 }, 0 } };
        AddCoin(180, 423, 0.0f); AddCoin(360, 420, 1.0f);
        AddCoin(575, 385, 2.0f); AddCoin(635, 385, 3.0f);
        AddCoin(805, 340, 1.5f); AddCoin(865, 340, 2.5f);
        AddCoin(1150, 415, 0.5f); AddCoin(1280, 415, 1.5f);
        AddCoin(1455, 360, 0.0f); AddCoin(1690, 315, 1.0f);
        AddCoin(1980, 405, 0.0f); AddCoin(2170, 350, 1.5f);
        AddCoin(2425, 415, 1.0f); AddCoin(2660, 415, 2.0f);
        AddEnemy(1120, 423, 990, 1320, 62.0f);
        AddEnemy(2410, 423, 2370, 2740, 72.0f);
    } else if (levelIndex == 1) {
        platforms = {
            { { 0, 470, 340, 100 } }, { { 390, 425, 150, 30 } },
            { { 590, 375, 150, 30 } }, { { 790, 420, 190, 30 } },
            { { 1030, 465, 280, 100 } }, { { 1360, 410, 155, 30 } },
            { { 1570, 355, 170, 30 } }, { { 1800, 405, 150, 30 } },
            { { 2010, 455, 270, 100 } }, { { 2340, 395, 160, 30 } },
            { { 2550, 450, 290, 100 } }
        };
        spikes = { { { 176, 444, 55, 26 }, 1 }, { { 1075, 439, 54, 26 }, 1 },
                   { { 2070, 429, 50, 26 }, 1 }, { { 2620, 424, 52, 26 }, 1 } };
        AddCoin(270, 420, 0.0f); AddCoin(430, 380, 1.0f);
        AddCoin(630, 330, 2.0f); AddCoin(840, 375, 3.0f);
        AddCoin(1180, 415, 1.5f); AddCoin(1415, 365, 0.5f);
        AddCoin(1630, 310, 2.0f); AddCoin(1850, 360, 3.0f);
        AddCoin(2150, 405, 1.0f); AddCoin(2380, 350, 0.0f);
        AddCoin(2690, 415, 2.0f);
        AddEnemy(830, 393, 800, 950, 83.0f);
        AddEnemy(1830, 378, 1810, 1915, 94.0f);
        AddEnemy(2580, 423, 2560, 2790, 108.0f);
    } else if (levelIndex == 2) {
        platforms = {
            { { 0, 470, 300, 100 } }, { { 355, 420, 130, 30 } },
            { { 530, 365, 130, 30 } }, { { 715, 410, 135, 30 } },
            { { 900, 350, 150, 30 } }, { { 1100, 455, 270, 100 } },
            { { 1420, 395, 135, 30 } }, { { 1600, 340, 145, 30 } },
            { { 1790, 395, 145, 30 } }, { { 1980, 455, 250, 100 } },
            { { 2285, 390, 145, 30 } }, { { 2480, 340, 145, 30 } },
            { { 2670, 450, 170, 100 } }
        };
        spikes = { { { 130, 444, 54, 26 }, 2 }, { { 1145, 429, 56, 26 }, 2 },
                   { { 2040, 429, 55, 26 }, 2 }, { { 2700, 424, 52, 26 }, 2 } };
        AddCoin(225, 420, 0.0f); AddCoin(390, 375, 1.0f);
        AddCoin(570, 320, 2.0f); AddCoin(755, 365, 3.0f);
        AddCoin(940, 305, 0.5f); AddCoin(1190, 405, 1.5f);
        AddCoin(1450, 350, 2.5f); AddCoin(1635, 295, 0.0f);
        AddCoin(1830, 350, 1.0f); AddCoin(2110, 405, 2.0f);
        AddCoin(2320, 345, 3.0f); AddCoin(2515, 295, 0.5f);
        AddEnemy(390, 393, 360, 470, 105.0f);
        AddEnemy(920, 323, 910, 1030, 110.0f);
        AddEnemy(1450, 368, 1430, 1520, 120.0f);
        AddEnemy(2320, 363, 2300, 2410, 125.0f);
    } else {
        // New stages use a longer handcrafted-feeling route generated from a fixed layout seed.
        // Gaps and height changes are deliberately bounded so every jump is possible.
        platforms.push_back({ { 0, 470, 300, 100 } });
    }

    auto buildRoute = [&](float routeStart, float startY) {
        const float routeEnd = WORLD_W - 260.0f;
        const float routeSpan = routeEnd - routeStart;
        const int segments = std::max(1, static_cast<int>(std::round(routeSpan / 225.0f)));
        const float spacing = routeSpan / static_cast<float>(segments);
        const float topHeights[8] = { 445.0f, 410.0f, 375.0f, 430.0f,
                                      355.0f, 400.0f, 370.0f, 440.0f };
        float previousY = startY;

        for (int i = 0; i < segments; ++i) {
            const float x = routeStart + spacing * static_cast<float>(i);
            const int pattern = (i * 3 + levelIndex * 5) % 8;
            float y = (i == 0) ? startY : topHeights[pattern];
            // Keep vertical jump height within a fair range.
            y = std::clamp(y, previousY - 54.0f, previousY + 46.0f);
            y = std::clamp(y, 345.0f, 455.0f);

            float gap = 34.0f + static_cast<float>((i * 7 + levelIndex * 11) % 25);
            if (difficulty == 2) gap += 8.0f;
            if (levelIndex >= 7) gap += 4.0f;
            gap = std::min(gap, spacing - 118.0f);
            float width = spacing - gap;
            if (difficulty == 2 && levelIndex >= 6) width = std::max(112.0f, width - 8.0f);

            Platform plat;
            plat.box = { x, y, width, (y >= 448.0f) ? 100.0f : 28.0f };
            if (levelIndex >= 3 && levelIndex % 3 == 0 && i > 1 && i % 5 == 2) {
                plat.moving = true;
                plat.originX = x;
                plat.travel = 15.0f + static_cast<float>(levelIndex - 2) * 2.0f;
                plat.moveSpeed = 0.65f + static_cast<float>(levelIndex) * 0.055f;
                plat.phase = static_cast<float>(i) * 0.83f;
            }
            platforms.push_back(plat);

            // Readable rewards are placed above safe portions of the platform.
            if (i % 4 != 2 || difficulty == 0) {
                AddCoin(x + width * 0.62f, y - 40.0f, static_cast<float>(i) * 0.9f);
                if (levelIndex >= 5 && i % 6 == 3) {
                    AddCoin(x + width * 0.72f, y - 72.0f, static_cast<float>(i) * 0.5f);
                }
            }

            int hazardEvery = difficulty == 0 ? 4 : (difficulty == 1 ? 3 : 2);
            if (levelIndex >= 6) hazardEvery = std::max(2, hazardEvery - 1);
            const bool hazardHere = i > 0 && ((i + levelIndex) % hazardEvery == 0);
            if (hazardHere && width >= 125.0f) {
                Spike sp;
                sp.box = { x + width * 0.25f, y - 26.0f,
                           std::min(58.0f, width * 0.42f), 26.0f };
                sp.type = (levelIndex + i) % 4;
                spikes.push_back(sp);
            }

            const int enemyEvery = difficulty == 0 ? 6 : (difficulty == 1 ? 4 : 3);
            if (i > 1 && (i + levelIndex) % enemyEvery == 0 && width >= 135.0f) {
                const float enemySpeed = 55.0f + static_cast<float>(levelIndex) * 5.0f +
                                         static_cast<float>(difficulty) * 13.0f;
                AddEnemy(x + width * 0.36f, y - 27.0f,
                         x + 2.0f, x + width - 2.0f, enemySpeed);
            }

            // Floating platforms on selected stages are clearly highlighted and move in a cycle.
            previousY = y;
        }

        const float endX = routeEnd;
        platforms.push_back({ { endX, 450.0f, WORLD_W - endX + 8.0f, 100.0f } });
        AddCoin(endX + 90.0f, 400.0f, 2.0f);
    };

    if (levelIndex < 3) buildRoute(2820.0f, 450.0f);
    else buildRoute(300.0f, 470.0f);

    // Level identity affects visible scenery and hazard art as well as geometry.
    // Extra challenge items are always drawn and match their collision rectangles.
    if (levelIndex >= 3) {
        const int extraCount = (levelIndex - 2) / 2;
        for (int n = 0; n < extraCount; ++n) {
            const float x = 470.0f + static_cast<float>(n) * 620.0f +
                            static_cast<float>(levelIndex % 3) * 54.0f;
            if (x < WORLD_W - 500.0f) {
                Spike sp;
                sp.box = { x, 444.0f, 48.0f, 26.0f };
                sp.type = (levelIndex + n + 1) % 4;
                spikes.push_back(sp);
            }
        }
    }

    player.pos = { 70.0f, 422.0f };
    player.vel = { 0.0f, 0.0f };
    player.grounded = false;
    cameraX = 0.0f;
    hurtFlash = 0.0f;
    damageNoticeTimer = 0.0f;
    landingTimer = 0.0f;
    runDustTimer = 0.0f;
    scoreNoticeTimer = 0.0f;
    scoreNoticeValue = 0;
    respawnTimer = 0.0f;
    screenShakeTimer = 0.0f;
    impactTimer = 0.0f;
    pendingGameOver = false;
}

void StartGame() {
    score = 0;
    player.lives = StartingLives();
    PlayFx(sfxMenu);
    LoadLevel(0);
    screen = Screen::Playing;
}

Rectangle PlayerRect() {
    return { player.pos.x, player.pos.y, PLAYER_W, PLAYER_H };
}

void TakeHit() {
    if (hurtFlash > 0.0f || respawnTimer > 0.0f) return;

    --player.lives;
    hurtFlash = 1.65f;
    damageNoticeTimer = 1.85f;
    respawnTimer = 0.68f;
    screenShakeTimer = 0.34f;
    impactTimer = 0.42f;
    pendingGameOver = player.lives <= 0;

    const Vector2 impact = { player.pos.x + PLAYER_W * 0.5f,
                             player.pos.y + PLAYER_H * 0.5f };
    impactScreenPos = { impact.x - cameraX, impact.y };
    SpawnParticles(impact, C_CORAL, 36);
    PlayFx(sfxHurt);

    // Visible knockback first; the player remains rendered and tinted instead of vanishing.
    player.vel.x = player.vel.x >= 0.0f ? -210.0f : 210.0f;
    player.vel.y = -285.0f;
    player.grounded = false;
}

void UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 300.0f * dt;
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p) { return p.life <= 0.0f; }), particles.end());
}

void UpdateGame(float dt) {
    hurtFlash = std::max(0.0f, hurtFlash - dt);
    damageNoticeTimer = std::max(0.0f, damageNoticeTimer - dt);
    scoreNoticeTimer = std::max(0.0f, scoreNoticeTimer - dt);
    screenShakeTimer = std::max(0.0f, screenShakeTimer - dt);
    impactTimer = std::max(0.0f, impactTimer - dt);
    UpdateParticles(dt);

    // Moving platforms are marked with an animated gold rim and carry a standing player.
    for (auto& p : platforms) {
        if (!p.moving) continue;
        const float oldX = p.box.x;
        p.box.x = p.originX + std::sin(gameClock * p.moveSpeed + p.phase) * p.travel;
        const float deltaX = p.box.x - oldX;
        const float playerBottom = player.pos.y + PLAYER_H;
        const bool standingOn = player.grounded &&
            std::fabs(playerBottom - p.box.y) <= 4.0f &&
            player.pos.x + PLAYER_W > oldX && player.pos.x < oldX + p.box.width;
        if (standingOn) player.pos.x += deltaX;
    }

    if (respawnTimer > 0.0f) {
        respawnTimer = std::max(0.0f, respawnTimer - dt);
        player.pos.x += player.vel.x * dt;
        player.pos.y += player.vel.y * dt;
        player.vel.y += GRAVITY * dt;

        if (respawnTimer <= 0.0f) {
            if (pendingGameOver) {
                pendingGameOver = false;
                screen = Screen::GameOver;
            } else {
                player.pos = { 70.0f, 422.0f };
                player.vel = { 0.0f, 0.0f };
                player.grounded = false;
                cameraX = 0.0f;
            }
        }
        return;
    }

    float move = 0.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) move += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) move -= 1.0f;

    if (IsGamepadAvailable(0)) {
        float axis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        if (std::fabs(axis) > 0.2f) move = axis;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) move = 1.0f;
        if (IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) move = -1.0f;
    }

    const Rectangle leftTouch  = { 34, 421, 68, 74 };
    const Rectangle rightTouch = { 112, 421, 68, 74 };
    const Rectangle jumpTouch  = { 848, 410, 78, 82 };
    if (Held(leftTouch)) move = -1.0f;
    if (Held(rightTouch)) move = 1.0f;
    move = std::clamp(move, -1.0f, 1.0f);

    bool jumpTouchDown = Held(jumpTouch);
    bool jumpPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) ||
                       IsKeyPressed(KEY_UP);
    if (IsGamepadAvailable(0) &&
        IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) jumpPressed = true;
    if (jumpTouchDown && !previousJumpTouch) jumpPressed = true;
    previousJumpTouch = jumpTouchDown;

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        screen = Screen::Paused;
        return;
    }

    const float baseSpeed = difficulty == 0 ? 245.0f : (difficulty == 2 ? 325.0f : 285.0f);
    const float levelSpeedBonus = static_cast<float>(levelIndex) * 3.5f;
    const float speed = baseSpeed + levelSpeedBonus;
    player.vel.x = move * speed;
    const float previousX = player.pos.x;
    player.pos.x += player.vel.x * dt;
    player.pos.x = std::clamp(player.pos.x, 0.0f, WORLD_W - PLAYER_W);

    Rectangle pRect = PlayerRect();
    for (const auto& p : platforms) {
        if (!CheckCollisionRecs(pRect, p.box)) continue;
        if (move > 0.0f) player.pos.x = p.box.x - PLAYER_W;
        else if (move < 0.0f) player.pos.x = p.box.x + p.box.width;
        pRect = PlayerRect();
    }

    if (std::fabs(move) > 0.05f) playerFacing = move > 0.0f ? 1.0f : -1.0f;
    landingTimer = std::max(0.0f, landingTimer - dt);

    if (jumpPressed && player.grounded) {
        player.vel.y = -JUMP_SPEED;
        player.grounded = false;
        SpawnFootDust({ player.pos.x + PLAYER_W * 0.5f,
                        player.pos.y + PLAYER_H - 2.0f }, 8);
        PlayFx(sfxJump);
    }

    const bool wasGrounded = player.grounded;
    const float oldBottom = player.pos.y + PLAYER_H;
    player.vel.y += GRAVITY * dt;
    player.pos.y += player.vel.y * dt;
    player.grounded = false;
    pRect = PlayerRect();

    for (const auto& p : platforms) {
        if (!CheckCollisionRecs(pRect, p.box)) continue;
        if (player.vel.y >= 0.0f && oldBottom <= p.box.y + 12.0f) {
            player.pos.y = p.box.y - PLAYER_H;
            player.vel.y = 0.0f;
            player.grounded = true;
        } else if (player.vel.y < 0.0f && player.pos.y >= p.box.y + p.box.height - 3.0f) {
            player.pos.y = p.box.y + p.box.height;
            player.vel.y = 40.0f;
        }
        pRect = PlayerRect();
    }

    if (!wasGrounded && player.grounded) {
        landingTimer = 0.15f;
        SpawnFootDust({ player.pos.x + PLAYER_W * 0.5f,
                        player.pos.y + PLAYER_H - 1.0f }, 9);
    }

    if (player.grounded && std::fabs(move) > 0.1f) {
        runDustTimer -= dt;
        if (runDustTimer <= 0.0f) {
            SpawnFootDust({ player.pos.x + PLAYER_W * 0.5f,
                            player.pos.y + PLAYER_H - 1.0f }, 3);
            runDustTimer = 0.16f;
        }
    } else runDustTimer = 0.0f;

    for (auto& coin : coins) {
        if (coin.taken) continue;
        Rectangle c = { coin.center.x - 10.0f, coin.center.y - 10.0f, 20.0f, 20.0f };
        if (CheckCollisionRecs(PlayerRect(), c)) {
            coin.taken = true;
            score += 10;
            highScore = std::max(highScore, score);
            scoreNoticeValue = 10;
            scoreNoticePos = coin.center;
            scoreNoticeTimer = 0.72f;
            SpawnParticles(coin.center, C_GOLD, 14);
            PlayFx(sfxCoin);
        }
    }

    for (const auto& s : spikes) {
        if (CheckCollisionRecs(PlayerRect(), s.box)) {
            TakeHit();
            return;
        }
    }

    if (screen == Screen::Playing) {
        for (auto& e : enemies) {
            if (!e.alive) continue;
            e.box.x += e.speed * static_cast<float>(e.direction) * dt;
            if (e.box.x < e.minX) { e.box.x = e.minX; e.direction = 1; }
            if (e.box.x + e.box.width > e.maxX) {
                e.box.x = e.maxX - e.box.width;
                e.direction = -1;
            }

            if (!CheckCollisionRecs(PlayerRect(), e.box)) continue;
            if (player.vel.y > 20.0f && oldBottom <= e.box.y + 14.0f) {
                e.alive = false;
                player.vel.y = -JUMP_SPEED * 0.58f;
                SpawnParticles({ e.box.x + 15.0f, e.box.y + 10.0f }, C_CORAL, 18);
                score += 25;
                highScore = std::max(highScore, score);
                scoreNoticeValue = 25;
                scoreNoticePos = { e.box.x + e.box.width * 0.5f, e.box.y };
                scoreNoticeTimer = 0.78f;
                PlayFx(sfxStomp);
            } else {
                TakeHit();
                return;
            }
        }
    }

    if (player.pos.y > VIEW_H + 110.0f && screen == Screen::Playing) {
        // Bring the fall impact into view so the life loss is never invisible off-screen.
        player.pos.y = VIEW_H - PLAYER_H - 18.0f;
        TakeHit();
        return;
    }

    const float targetCamera = std::clamp(player.pos.x - VIEW_W * 0.36f,
                                           0.0f, WORLD_W - VIEW_W);
    cameraX += (targetCamera - cameraX) * std::min(1.0f, dt * 5.0f);

    if (screen == Screen::Playing && player.pos.x > WORLD_W - 135.0f) {
        SpawnParticles({ WORLD_W - 100.0f, 385.0f }, C_MINT, 30);
        PlayFx(sfxLevel);
        if (levelIndex < LEVEL_COUNT - 1) {
            LoadLevel(levelIndex + 1);
        } else {
            screen = Screen::Victory;
        }
    }

    (void)previousX;
}

void DrawBackground(float camX) {
    if (backgroundTexture.id != 0) {
        // Preserve the supplied GIF's aspect ratio and pan a little as the player advances.
        const float texW = static_cast<float>(backgroundTexture.width);
        const float texH = static_cast<float>(backgroundTexture.height);
        const float scale = std::max(static_cast<float>(VIEW_W) / texW,
                                      static_cast<float>(VIEW_H) / texH);
        const float drawW = texW * scale;
        const float drawH = texH * scale;
        const float spareX = std::max(0.0f, drawW - VIEW_W);
        const float progress = std::clamp(camX / (WORLD_W - VIEW_W), 0.0f, 1.0f);
        const Rectangle source = { 0.0f, 0.0f, texW, texH };
        const Rectangle dest = { -spareX * progress, (VIEW_H - drawH) * 0.5f,
                                 drawW, drawH };
        DrawTexturePro(backgroundTexture, source, dest, { 0.0f, 0.0f },
                       0.0f, WHITE);
        // One shared animated GIF, with a subtle biome tint so each level feels different.
        static const Color tint[LEVEL_COUNT]={
            {14,64,74,24},{5,105,135,28},{117,75,38,26},{32,104,55,24},{70,72,155,33},
            {186,104,36,25},{63,141,205,32},{200,48,27,31},{105,66,163,35},{196,130,44,33}
        };
        DrawRectangle(0,0,VIEW_W,VIEW_H,tint[levelIndex]);
        DrawRectangleGradientV(0,0,VIEW_W,VIEW_H,Color{12,32,58,18},Color{9,22,37,55});
        return;
    }

    // Fallback artwork is used only if a user builds without the supplied files.
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 17, 28, 62, 255 },
                           Color{ 87, 158, 190, 255 });
    for (int i = 0; i < 42; ++i) {
        const int x = (i * 137 + 41) % VIEW_W;
        const int y = (i * 71 + 23) % 260;
        const unsigned char alpha = static_cast<unsigned char>(110 + (i % 4) * 35);
        DrawCircle(x, y, (i % 7 == 0) ? 2.0f : 1.0f, Color{ 231, 243, 255, alpha });
    }
    DrawCircle(760, 105, 40, Color{ 255, 221, 148, 50 });
    DrawCircle(760, 105, 28, Color{ 255, 228, 165, 220 });

    for (int i = 0; i < 9; ++i) {
        float x = std::fmod(i * 190.0f - camX * 0.18f, 1900.0f);
        if (x < -220.0f) x += 1900.0f;
        const float peak = 250.0f + static_cast<float>((i * 37) % 70);
        DrawTriangle({ x - 120, 420 }, { x + 50, peak }, { x + 225, 420 },
                     Color{ 55, 105, 142, 180 });
        DrawTriangle({ x + 10, 420 }, { x + 130, peak + 40 }, { x + 260, 420 },
                     Color{ 43, 85, 125, 190 });
    }
    DrawRectangle(0, 420, VIEW_W, 120, Color{ 25, 58, 78, 150 });
    for (int i = 0; i < 24; ++i) {
        float x = std::fmod(i * 85.0f - camX * 0.33f, 2040.0f);
        if (x < -80) x += 2040.0f;
        const float h = static_cast<float>(45 + (i * 19) % 72);
        DrawRectangle(static_cast<int>(x), static_cast<int>(470 - h),
                      28 + (i % 3) * 8, static_cast<int>(h),
                      Color{ 26, 62, 81, 155 });
        DrawRectangle(static_cast<int>(x + 5), static_cast<int>(480 - h),
                      4, 4, Color{ 255, 221, 132, 90 });
    }
}

void DrawPlatform(const Platform& p) {
    const Rectangle r={p.box.x-cameraX,p.box.y,p.box.width,p.box.height};
    if(r.x>VIEW_W||r.x+r.width<0)return;

    static const Color tops[LEVEL_COUNT]={
        {83,225,143,255},{83,213,223,255},{221,191,116,255},{105,209,127,255},
        {139,197,255,255},{234,177,106,255},{185,230,255,255},{255,113,77,255},
        {185,151,255,255},{255,217,105,255}
    };
    static const Color soils[LEVEL_COUNT]={
        {75,49,48,255},{41,76,91,255},{91,73,57,255},{37,73,56,255},{48,59,103,255},
        {100,62,38,255},{55,82,111,255},{81,39,40,255},{47,44,81,255},{76,55,42,255}
    };
    const Color top=p.moving?Color{255,220,118,255}:tops[levelIndex];
    const Color soil=p.moving?Color{56,68,91,255}:soils[levelIndex];
    DrawRectangleRounded({r.x,r.y+6,r.width,r.height},.08f,5,Color{8,16,30,170});
    DrawRectangleRec(r,soil);
    DrawRectangle((int)r.x,(int)(r.y+9),(int)r.width,(int)std::max(0.0f,r.height-9),Color{35,42,57,255});
    DrawRectangle((int)r.x,(int)r.y,(int)r.width,p.moving?8:10,top);
    DrawRectangle((int)r.x,(int)(r.y+10),(int)r.width,4,
                  Color{(unsigned char)(top.r*.55f),(unsigned char)(top.g*.55f),(unsigned char)(top.b*.55f),255});
    for(float x=r.x+13;x<r.x+r.width-8;x+=37)
        DrawRectangle((int)x,(int)(r.y+24),17,4,Color{143,92,71,220});
    DrawRectangleLinesEx(r,1.5f,p.moving?Color{255,245,197,255}:Color{29,36,48,255});
    if(p.moving) {
        const float cy=r.y+17;
        DrawLineEx({r.x+12,cy},{r.x+23,cy},2,Color{255,244,190,255});
        DrawTriangle({r.x+23,cy},{r.x+18,cy-4},{r.x+18,cy+4},Color{255,244,190,255});
        DrawLineEx({r.x+r.width-12,cy},{r.x+r.width-23,cy},2,Color{255,244,190,255});
    }
}
void DrawCoin(const Coin& coin) {
    if (coin.taken) return;
    const float bob = std::sin(gameClock * 3.4f + coin.phase) * 4.0f;
    const Vector2 c = { coin.center.x - cameraX, coin.center.y + bob };
    if (c.x < -30 || c.x > VIEW_W + 30) return;
    DrawCircleV({ c.x, c.y }, 13, Color{ 255, 199, 65, 42 });
    DrawCircleV(c, 9, C_GOLD);
    DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), 9, Color{ 255, 244, 180, 255 });
    DrawCircleV({ c.x - 2.0f, c.y - 2.0f }, 2.7f, Color{ 255, 248, 205, 255 });
}

void DrawSpike(const Spike& spike) {
    const float x = spike.box.x - cameraX, y = spike.box.y, w = spike.box.width;
    if (x > VIEW_W || x + w < 0) return;

    // Each hazard has a unique visible silhouette, aligned to its collision box.
    if (spike.type == 1) {
        DrawEllipse((int)(x + w*0.5f), (int)(y+22), w*0.58f, 8, Color{18,49,48,255});
        DrawCircleV({x+w*0.22f,y+15},13,Color{37,127,84,255});
        DrawCircleV({x+w*0.48f,y+11},16,Color{66,178,98,255});
        DrawCircleV({x+w*0.75f,y+15},13,Color{37,127,84,255});
        DrawTriangle({x+w*.30f,y+9},{x+w*.39f,y+1},{x+w*.45f,y+11},Color{229,255,181,255});
        DrawTriangle({x+w*.60f,y+10},{x+w*.71f,y+1},{x+w*.76f,y+12},Color{229,255,181,255});
    } else if (spike.type == 2) {
        DrawCircleV({x+w*.5f,y+17},23,Color{107,94,255,40});
        DrawTriangle({x+2,y+26},{x+w*.30f,y+7},{x+w*.45f,y+26},Color{82,80,205,255});
        DrawTriangle({x+w*.25f,y+26},{x+w*.52f,y-1},{x+w*.74f,y+26},Color{116,194,255,255});
        DrawTriangle({x+w*.55f,y+26},{x+w*.82f,y+8},{x+w-2,y+26},Color{84,110,225,255});
        DrawLineEx({x+w*.52f,y+3},{x+w*.52f,y+22},2,Color{230,251,255,255});
    } else if (spike.type == 3) {
        DrawCircleV({x+w*.5f,y+19},23,Color{255,74,30,42});
        DrawTriangle({x+2,y+26},{x+w*.24f,y+9},{x+w*.42f,y+26},Color{196,48,40,255});
        DrawTriangle({x+w*.25f,y+26},{x+w*.53f,y+1+std::sin(gameClock*8)*2},{x+w*.70f,y+26},Color{255,109,48,255});
        DrawTriangle({x+w*.39f,y+26},{x+w*.53f,y+10},{x+w*.63f,y+26},Color{255,219,119,255});
        DrawRectangle((int)x,(int)(y+24),(int)w,4,Color{118,37,42,255});
    } else {
        const int n=std::max(1,(int)(w/17.0f)); const float sw=w/n;
        DrawRectangleRounded({x-2,y+20,w+4,8},.25f,5,Color{91,25,45,255});
        for(int i=0;i<n;++i) {
            const float x1=x+i*sw;
            DrawTriangle({x1,y+26},{x1+sw*.5f,y-2},{x1+sw,y+26},Color{111,28,54,255});
            DrawTriangle({x1+1,y+25},{x1+sw*.5f,y+2},{x1+sw-1,y+25},Color{255,88,105,255});
            DrawTriangle({x1+sw*.20f,y+23},{x1+sw*.47f,y+7},{x1+sw*.56f,y+23},Color{255,190,184,255});
        }
    }
}
void DrawEnemy(const Enemy& e) {
    if (!e.alive) return;
    const float x = e.box.x - cameraX;
    if (x > VIEW_W || x + e.box.width < 0) return;

    DrawEllipse(static_cast<int>(x + e.box.width * 0.5f),
                static_cast<int>(e.box.y + e.box.height + 2.0f),
                23.0f, 5.0f, Color{ 9, 17, 30, 115 });
    DrawRectangleRounded({ x, e.box.y + 3.0f, e.box.width, e.box.height - 3.0f },
                         0.30f, 8, Color{ 96, 24, 57, 255 });
    DrawRectangleRounded({ x + 2.0f, e.box.y, e.box.width - 4.0f, e.box.height - 7.0f },
                         0.30f, 8, C_CORAL);
    DrawRectangleRoundedLinesEx({ x + 2.0f, e.box.y, e.box.width - 4.0f,
                                  e.box.height - 7.0f }, 0.30f, 8, 2.0f,
                                Color{ 255, 209, 202, 255 });
    DrawCircle(static_cast<int>(x + 12), static_cast<int>(e.box.y + 12), 5.0f, WHITE);
    DrawCircle(static_cast<int>(x + 25), static_cast<int>(e.box.y + 12), 5.0f, WHITE);
    DrawCircle(static_cast<int>(x + 13 + e.direction * 1.5f),
               static_cast<int>(e.box.y + 12), 2.2f, C_INK);
    DrawCircle(static_cast<int>(x + 26 + e.direction * 1.5f),
               static_cast<int>(e.box.y + 12), 2.2f, C_INK);
    DrawLineEx({ x + 9.0f, e.box.y + 23.0f }, { x + 27.0f, e.box.y + 23.0f },
               2.0f, Color{ 92, 24, 49, 255 });
}

void DrawPlayerFallback() {
    const float x = player.pos.x - cameraX;
    const bool damaged = hurtFlash > 0.0f || respawnTimer > 0.0f;
    const Color body = damaged ? C_CORAL : C_MINT;
    DrawEllipse(static_cast<int>(x + PLAYER_W * 0.5f),
                static_cast<int>(player.pos.y + PLAYER_H + 2.0f),
                23.0f, 6.0f, Color{ 8, 15, 27, 135 });
    DrawRectangleRounded({ x + 2.0f, player.pos.y + 16.0f,
                           PLAYER_W - 4.0f, PLAYER_H - 16.0f },
                         0.26f, 8, body);
    DrawRectangleRoundedLinesEx({ x + 2.0f, player.pos.y + 16.0f,
                                  PLAYER_W - 4.0f, PLAYER_H - 16.0f },
                                0.26f, 8, 2.0f, Color{ 18, 47, 69, 255 });
    DrawCircleV({ x + PLAYER_W * 0.5f, player.pos.y + 13.0f }, 12.0f,
                Color{ 226, 255, 241, 255 });
    DrawCircleV({ x + PLAYER_W * 0.5f - 4.0f, player.pos.y + 13.0f }, 2.0f, C_INK);
    DrawCircleV({ x + PLAYER_W * 0.5f + 4.0f, player.pos.y + 13.0f }, 2.0f, C_INK);
    if (damaged) DrawUIText("HIT", static_cast<int>(x), static_cast<int>(player.pos.y - 18), 14, C_CORAL);
}

void DrawPlayer() {
    if (characterAtlas.id == 0) {
        DrawPlayerFallback();
        return;
    }

    const bool damaged = hurtFlash > 0.0f || respawnTimer > 0.0f;
    int row = 0;
    int frame = 0;
    const float horizontalSpeed = std::fabs(player.vel.x);

    if (damaged) {
        row = 3;
        frame = player.vel.y < 0.0f ? 2 : 3;
    } else if (!player.grounded) {
        row = 3;
        if (player.vel.y < -90.0f) frame = 2;       // rising jump, arms up
        else if (player.vel.y > 100.0f) frame = 3;  // falling / air pose
        else frame = 1;                             // crouch at the jump apex
    } else if (landingTimer > 0.0f) {
        row = 3;
        frame = 4;                                 // landing pose
    } else if (horizontalSpeed > 40.0f) {
        row = horizontalSpeed < 282.0f ? 1 : 2;    // walk or run strip
        const float rate = row == 1 ? 8.0f : 12.0f;
        frame = static_cast<int>(gameClock * rate) % 5;
    } else {
        row = 0;
        frame = static_cast<int>(gameClock * 3.0f) % 5;
    }

    const Rectangle sourceBase = {
        static_cast<float>(frame * 64),
        static_cast<float>(row * 80),
        64.0f, 80.0f
    };
    Rectangle source = sourceBase;
    if (playerFacing < 0.0f) {
        source.x += source.width;
        source.width = -source.width;
    }

    constexpr float spriteW = 76.0f;
    constexpr float spriteH = 100.0f;
    const float screenX = player.pos.x - cameraX;
    const float bottom = player.pos.y + PLAYER_H;
    const Rectangle dest = {
        screenX + (PLAYER_W - spriteW) * 0.5f,
        bottom - spriteH,
        spriteW, spriteH
    };

    DrawEllipse(static_cast<int>(screenX + PLAYER_W * 0.5f),
                static_cast<int>(bottom + 2.0f), 24.0f, 6.0f,
                Color{ 8, 15, 27, 130 });

    const Color tint = damaged ? Color{ 255, 160, 170, 255 } : WHITE;
    DrawTexturePro(characterAtlas, source, dest, { 0.0f, 0.0f }, 0.0f, tint);

    // Never blink the character out of view. Add an explicit hit marker instead.
    if (damaged) {
        const float ring = 32.0f + std::sin(gameClock * 36.0f) * 2.5f;
        DrawCircleLines(static_cast<int>(screenX + PLAYER_W * 0.5f),
                        static_cast<int>(bottom - spriteH * 0.47f),
                        ring, Color{ 255, 81, 111, 230 });
        DrawUIText("HIT", static_cast<int>(screenX - 1.0f),
                 static_cast<int>(bottom - spriteH - 8.0f), 14, Color{ 255, 225, 224, 255 });
    }
}

void DrawGoal() {
    const float x = WORLD_W - 105.0f - cameraX;
    DrawRectangle(static_cast<int>(x), 360, 5, 90, Color{ 234, 244, 255, 240 });
    DrawTriangle({ x + 5, 360 }, { x + 63, 378 }, { x + 5, 397 }, C_MINT);
    DrawCircle(static_cast<int>(x + 2), 355, 6, C_GOLD);
    DrawUIText("GOAL", static_cast<int>(x - 12), 408, 12, C_WHITE_90);
}

void DrawParticles() {
    for (const auto& p : particles) {
        float alpha = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
        Color c = p.color;
        c.a = static_cast<unsigned char>(255.0f * alpha);
        DrawCircleV({ p.pos.x - cameraX, p.pos.y }, p.radius * alpha, c);
    }
}

void DrawTouchControls() {
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
    const Rectangle left  = { 34, 421, 68, 74 };
    const Rectangle right = { 112, 421, 68, 74 };
    const Rectangle jump  = { 848, 410, 78, 82 };
    const Color base = { 12, 27, 48, 106 };
    DrawCircleV({ left.x + 34, left.y + 34 }, 31, base);
    DrawCircleV({ right.x + 34, right.y + 34 }, 31, base);
    DrawCircleV({ jump.x + 39, jump.y + 39 }, 36, Color{ 32, 102, 111, 150 });
    DrawCircleLines(static_cast<int>(left.x + 34), static_cast<int>(left.y + 34),
                    31, Color{ 214, 239, 255, 145 });
    DrawCircleLines(static_cast<int>(right.x + 34), static_cast<int>(right.y + 34),
                    31, Color{ 214, 239, 255, 145 });
    DrawCircleLines(static_cast<int>(jump.x + 39), static_cast<int>(jump.y + 39),
                    36, Color{ 128, 255, 208, 190 });
    DrawTriangle({ left.x + 22, left.y + 34 }, { left.x + 43, left.y + 19 },
                 { left.x + 43, left.y + 49 }, C_WHITE_90);
    DrawTriangle({ right.x + 46, right.y + 34 }, { right.x + 25, right.y + 19 },
                 { right.x + 25, right.y + 49 }, C_WHITE_90);
    DrawUIText("JUMP", static_cast<int>(jump.x + 15), static_cast<int>(jump.y + 31),
             16, C_WHITE_90);
#endif
}

void DrawHeart(float x, float y, Color c) {
    DrawCircleV({ x - 4.0f, y }, 5.0f, c);
    DrawCircleV({ x + 4.0f, y }, 5.0f, c);
    DrawTriangle({ x - 9.0f, y + 2.0f }, { x + 9.0f, y + 2.0f },
                 { x, y + 12.0f }, c);
}

void DrawScoreNotice() {
    if (scoreNoticeTimer <= 0.0f) return;
    const float elapsed = 0.72f - scoreNoticeTimer;
    const float x = scoreNoticePos.x - cameraX;
    const float y = scoreNoticePos.y - 18.0f - elapsed * 40.0f;
    const unsigned char alpha = static_cast<unsigned char>(
        255.0f * std::clamp(scoreNoticeTimer / 0.20f, 0.0f, 1.0f));
    DrawUIText(TextFormat("+%d", scoreNoticeValue), static_cast<int>(x - 8.0f),
             static_cast<int>(y), 20, Color{ 255, 232, 133, alpha });
}

void DrawImpactEffect() {
    if (impactTimer <= 0.0f) return;
    const float age = 0.42f - impactTimer;
    const float radius = 10.0f + age * 210.0f;
    const unsigned char alpha = static_cast<unsigned char>(
        255.0f * std::clamp(impactTimer / 0.12f, 0.0f, 1.0f));
    DrawCircleLines(static_cast<int>(impactScreenPos.x),
                    static_cast<int>(impactScreenPos.y), radius,
                    Color{ 255, 92, 116, alpha });
    DrawCircleLines(static_cast<int>(impactScreenPos.x),
                    static_cast<int>(impactScreenPos.y), radius * 0.62f,
                    Color{ 255, 207, 151, alpha });
    for (int i = 0; i < 8; ++i) {
        const float a = i * (PI / 4.0f) + age * 3.0f;
        const Vector2 from = { impactScreenPos.x + std::cos(a) * (radius * 0.55f),
                               impactScreenPos.y + std::sin(a) * (radius * 0.55f) };
        const Vector2 to = { impactScreenPos.x + std::cos(a) * (radius * 0.88f),
                             impactScreenPos.y + std::sin(a) * (radius * 0.88f) };
        DrawLineEx(from, to, 3.0f, Color{ 255, 222, 168, alpha });
    }
}

void DrawDamageFeedback() {
    if (damageNoticeTimer <= 0.0f ||
        (screen != Screen::Playing && screen != Screen::Paused)) return;
    const float fade = std::clamp(damageNoticeTimer / 0.24f, 0.0f, 1.0f);
    const unsigned char alpha = static_cast<unsigned char>(250.0f * fade);
    const float edge = std::clamp(damageNoticeTimer / 0.48f, 0.0f, 1.0f);
    const unsigned char edgeAlpha = static_cast<unsigned char>(85.0f * edge);
    DrawRectangle(0, 0, VIEW_W, 12, Color{ 255, 50, 83, edgeAlpha });
    DrawRectangle(0, VIEW_H - 12, VIEW_W, 12, Color{ 255, 50, 83, edgeAlpha });
    DrawRectangle(0, 0, 12, VIEW_H, Color{ 255, 50, 83, edgeAlpha });
    DrawRectangle(VIEW_W - 12, 0, 12, VIEW_H, Color{ 255, 50, 83, edgeAlpha });
    DrawRectangleRounded({ 304, 88, 352, 78 }, 0.18f, 12,
                         Color{ 60, 15, 34, static_cast<unsigned char>(235.0f * fade) });
    DrawRectangleRoundedLinesEx({ 304, 88, 352, 78 }, 0.18f, 12, 3.0f,
                                Color{ 255, 90, 117, alpha });
    DrawTriangle({ 329, 105 }, { 351, 145 }, { 307, 145 },
                 Color{ 255, 91, 110, alpha });
    DrawUIText("!", 324, 113, 25, Color{ 54, 18, 33, 255 });
    DrawUIText(pendingGameOver ? "FINAL HIT!" : "DAMAGE!  -1 LIFE",
             365, 101, 24, Color{ 255, 164, 173, alpha });
    DrawUIText(pendingGameOver ? "No lives left" : "Knocked back - respawning",
             366, 133, 13, C_WHITE_90);
}

void DrawHud() {
    DrawRectangleRounded({ 16, 15, 288, 58 }, 0.18f, 8, Color{ 13, 26, 45, 205 });
    DrawUIText(TextFormat("LEVEL %02d / %02d", levelIndex + 1, LEVEL_COUNT), 30, 21, 13,
             Color{ 159, 190, 220, 255 });
    DrawUIText(LevelName(levelIndex), 30, 36, 10,
             Color{ 159, 216, 208, 255 });
    DrawUIText(TextFormat("SCORE %04d", score), 30, 48, 18, C_GOLD);
    DrawUIText("LIVES", 193, 23, 12, Color{ 159, 190, 220, 255 });
    for (int i = 0; i < StartingLives(); ++i) {
        const Color heart = i < player.lives ? C_CORAL : Color{ 75, 82, 103, 255 };
        DrawHeart(204.0f + i * 17.0f, 50.0f, heart);
    }

    DrawRectangleRounded({ 768, 17, 174, 42 }, 0.20f, 7, Color{ 13, 26, 45, 205 });
    DrawUIText(DifficultyName(), 785, 29, 15, C_WHITE_90);
    if (Button({ 891, 20, 39, 36 }, "II")) screen = Screen::Paused;
    DrawUIText("ESC / P = PAUSE", 766, 65, 12, Color{ 225, 239, 250, 190 });
}

void DrawGame() {
    DrawBackground(cameraX);
    for (const auto& p : platforms) DrawPlatform(p);
    for (const auto& s : spikes) DrawSpike(s);
    for (const auto& c : coins) DrawCoin(c);
    for (const auto& e : enemies) DrawEnemy(e);
    DrawGoal();
    DrawPlayer();
    DrawParticles();
    // The shock ring is intentionally on top of the player so impacts are readable.
    DrawImpactEffect();
    DrawScoreNotice();
    DrawHud();
    DrawTouchControls();
    DrawDamageFeedback();
}

void DrawMenuHero(float x, float y) {
    const float bob = std::sin(gameClock * 2.8f) * 3.0f;
    DrawCircle(static_cast<int>(x), static_cast<int>(y + 32.0f),
               48.0f, Color{ 91, 245, 190, 20 });
    DrawCircleLines(static_cast<int>(x), static_cast<int>(y + 32.0f),
                    40.0f + std::sin(gameClock * 2.0f) * 2.0f,
                    Color{ 117, 255, 204, 85 });

    if (characterAtlas.id != 0) {
        const int idleFrame = static_cast<int>(gameClock * 3.0f) % 5;
        const Rectangle source = { static_cast<float>(idleFrame * 64), 0.0f, 64.0f, 80.0f };
        const Rectangle dest = { x - 40.0f, y - 15.0f + bob, 80.0f, 100.0f };
        DrawTexturePro(characterAtlas, source, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
    } else {
        DrawCircleV({ x, y + 28.0f + bob }, 18.0f, C_MINT);
        DrawCircleV({ x - 5.0f, y + 27.0f + bob }, 3.0f, WHITE);
        DrawCircleV({ x + 5.0f, y + 27.0f + bob }, 3.0f, WHITE);
    }

    const float coinY = y + 6.0f + std::sin(gameClock * 3.0f) * 4.0f;
    DrawCircleV({ x + 75.0f, coinY }, 14.0f, Color{ 255, 207, 80, 35 });
    DrawCircleV({ x + 75.0f, coinY }, 8.0f, C_GOLD);
    DrawCircleLines(static_cast<int>(x + 75.0f), static_cast<int>(coinY),
                    8.0f, Color{ 255, 249, 198, 255 });
}
void DrawTitle() {
    DrawBackground(0.0f);
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 8, 16, 38, 218 }, Color{ 10, 25, 43, 208 });

    // Left hero panel.
    DrawRectangleRounded({ 42, 42, 455, 456 }, 0.06f, 12,
                         Color{ 12, 26, 48, 165 });
    DrawRectangleRoundedLinesEx({ 42, 42, 455, 456 }, 0.06f, 12, 1.5f,
                                Color{ 88, 136, 164, 150 });
    DrawRectangleRounded({ 72, 71, 205, 28 }, 0.45f, 8,
                         Color{ 35, 96, 91, 220 });
    DrawUIText("A TINY PLATFORM ADVENTURE", 84, 79, 13, C_MINT);
    DrawUIText("TINY", 75, 120, 54, C_WHITE_90);
    DrawUIText("TRAILS", 73, 169, 58, C_MINT);
    DrawUIText("JUMP  /  COLLECT  /  SURVIVE", 80, 239, 15,
             Color{ 201, 224, 239, 255 });

    // Floating islands make the game identity readable before pressing Play.
    DrawRectangleRounded({ 84, 365, 220, 21 }, 0.24f, 7,
                         Color{ 59, 114, 102, 255 });
    DrawRectangleRounded({ 84, 365, 220, 7 }, 0.25f, 7, C_MINT);
    DrawRectangleRounded({ 328, 326, 95, 18 }, 0.24f, 7,
                         Color{ 59, 114, 102, 255 });
    DrawRectangleRounded({ 328, 326, 95, 6 }, 0.25f, 7, C_MINT);
    DrawMenuHero(201.0f, 295.0f);
    DrawCircleV({ 353, 297 }, 14, Color{ 255, 207, 80, 35 });
    DrawCircleV({ 353, 297 }, 8, C_GOLD);
    DrawCircleLines(353, 297, 8, Color{ 255, 249, 198, 255 });
    DrawUIText("10 LEVELS", 81, 427, 19, C_GOLD);
    DrawUIText("Easy, Medium and Hard difficulty", 81, 454, 15,
             Color{ 177, 203, 223, 255 });
    DrawUIText("KEYBOARD  /  CONTROLLER  /  TOUCH", 81, 475, 13,
             Color{ 145, 180, 205, 255 });

    // Right-hand action card.
    DrawRectangleRounded({ 527, 42, 391, 456 }, 0.07f, 12,
                         Color{ 10, 21, 40, 238 });
    DrawRectangleRoundedLinesEx({ 527, 42, 391, 456 }, 0.07f, 12, 2.0f,
                                Color{ 79, 121, 158, 220 });
    DrawUIText("MAIN MENU", 570, 76, 23, C_WHITE_90);
    DrawUIText("Choose your next move", 570, 106, 14,
             Color{ 160, 191, 216, 255 });
    DrawRectangle(570, 132, 305, 1, Color{ 77, 104, 137, 190 });

    if (Button({ 570, 154, 305, 54 }, "PLAY")) StartGame();
    if (Button({ 570, 223, 305, 54 },
               TextFormat("DIFFICULTY: %s", DifficultyName()))) {
        difficulty = (difficulty + 1) % 3;
        PlayFx(sfxMenu);
    }
    if (Button({ 570, 292, 305, 54 }, "HOW TO PLAY")) {
        PlayFx(sfxMenu);
        screen = Screen::Controls;
    }

    DrawRectangleRounded({ 570, 372, 305, 71 }, 0.12f, 8,
                         Color{ 22, 42, 64, 255 });
    DrawUIText("BEST RUN", 590, 385, 11, Color{ 151, 183, 207, 255 });
    DrawUIText(TextFormat("%04d", highScore), 590, 402, 25, C_GOLD);
    DrawUIText("Collect coins. Reach the flag.", 688, 402, 12, C_WHITE_90);
    DrawUIText("ESC / P pauses during a run", 570, 460, 12,
             Color{ 153, 183, 207, 255 });
}

void DrawControlsScreen() {
    DrawBackground(0.0f);
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 10, 17, 38, 165 }, Color{ 13, 28, 48, 110 });
    TextCentered("TINY TRAILS", 76, 38, C_WHITE_90);
    DrawRectangleRounded({ 225, 150, 510, 290 }, 0.06f, 8, Color{ 12, 23, 43, 248 });
    TextCentered("HOW TO PLAY", 174, 28, C_MINT);
    DrawUIText("MOVE", 275, 226, 17, C_GOLD);
    DrawUIText("A / D or arrow keys", 425, 226, 17, C_WHITE_90);
    DrawUIText("JUMP", 275, 263, 17, C_GOLD);
    DrawUIText("Space / W / Up", 425, 263, 17, C_WHITE_90);
    DrawUIText("CONTROLLER", 275, 300, 17, C_GOLD);
    DrawUIText("D-pad or left stick + A button", 425, 300, 15, C_WHITE_90);
    DrawUIText("MOBILE", 275, 337, 17, C_GOLD);
    DrawUIText("Use the on-screen arrows and JUMP", 425, 337, 15, C_WHITE_90);
    if (Button({ 370, 381, 220, 42 }, "BACK")) screen = Screen::Menu;
}

void DrawOverlay(const char* title, const char* subtitle) {
    DrawGame();
    DrawRectangle(0, 0, VIEW_W, VIEW_H, Color{ 5, 12, 25, 175 });
    DrawRectangleRounded({ 268, 144, 424, 252 }, 0.07f, 10, Color{ 18, 31, 54, 248 });
    DrawRectangleRoundedLinesEx({ 268, 144, 424, 252 }, 0.07f, 10, 2,
                              Color{ 83, 130, 163, 220 });
    TextCentered(title, 177, 32, C_WHITE_90);
    TextCentered(subtitle, 220, 15, Color{ 177, 201, 223, 255 });
}

void DrawPauseScreen() {
    DrawOverlay("PAUSED", "Take a breath. The trail will wait.");
    if (Button({ 350, 262, 260, 42 }, "RESUME")) screen = Screen::Playing;
    if (Button({ 350, 314, 260, 38 }, "RESTART LEVEL")) {
        player.lives = StartingLives();
        LoadLevel(levelIndex);
        screen = Screen::Playing;
    }
    if (Button({ 350, 362, 260, 28 }, "MAIN MENU")) screen = Screen::Menu;
}

void DrawGameOverScreen() {
    DrawOverlay("GAME OVER", TextFormat("Final score: %d", score));
    if (Button({ 350, 270, 260, 43 }, "TRY AGAIN")) StartGame();
    if (Button({ 350, 326, 260, 40 }, "MAIN MENU")) screen = Screen::Menu;
}

void DrawVictoryScreen() {
    DrawOverlay("YOU DID IT!", TextFormat("Trail complete  |  Score %d", score));
    if (Button({ 350, 270, 260, 43 }, "PLAY AGAIN")) StartGame();
    if (Button({ 350, 326, 260, 40 }, "MAIN MENU")) screen = Screen::Menu;
}
} // namespace

int main() {
#if !defined(__ANDROID__) && !defined(PLATFORM_ANDROID) && !defined(PLATFORM_WEB)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
#else
    SetConfigFlags(FLAG_VSYNC_HINT);
#endif
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(VIEW_W, VIEW_H, "Tiny Trails - Mini Platformer");
    SetTargetFPS(60);
    SetWindowMinSize(800, 450);
    LoadGameAssets();
    LoadReadableFont();
    InitGameAudio();
    canvas = LoadRenderTexture(VIEW_W, VIEW_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose() && !exitRequested) {
        const float dt = std::min(GetFrameTime(), 0.033f);
        gameClock += dt;
        UpdateBackgroundAnimation(dt);
        UpdatePointer();

        if (screen == Screen::Menu) {
            if (IsKeyPressed(KEY_ENTER) || IsGamepadAvailable(0) &&
                IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
                StartGame();
            }
        } else if (screen == Screen::Playing) {
            UpdateGame(dt);
        } else if (screen == Screen::Paused) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) screen = Screen::Playing;
        } else if (screen == Screen::Controls) {
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        } else if (screen == Screen::GameOver || screen == Screen::Victory) {
            if (IsKeyPressed(KEY_ESCAPE)) screen = Screen::Menu;
        }

        BeginTextureMode(canvas);
        ClearBackground(C_INK);
        switch (screen) {
            case Screen::Menu: DrawTitle(); break;
            case Screen::Controls: DrawControlsScreen(); break;
            case Screen::Playing: DrawGame(); break;
            case Screen::Paused: DrawPauseScreen(); break;
            case Screen::GameOver: DrawGameOverScreen(); break;
            case Screen::Victory: DrawVictoryScreen(); break;
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(Color{ 6, 12, 25, 255 });
        const Rectangle source = { 0.0f, 0.0f, static_cast<float>(canvas.texture.width),
                                   -static_cast<float>(canvas.texture.height) };
        const float shake = screenShakeTimer > 0.0f ? screenShakeTimer * 11.0f : 0.0f;
        const float shakeX = shake > 0.0f ? std::sin(gameClock * 91.0f) * shake * viewScale : 0.0f;
        const float shakeY = shake > 0.0f ? std::cos(gameClock * 107.0f) * shake * viewScale : 0.0f;
        const Rectangle dest = { viewOffsetX + shakeX, viewOffsetY + shakeY,
                                 VIEW_W * viewScale, VIEW_H * viewScale };
        DrawTexturePro(canvas.texture, source, dest, { 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    UnloadGameAssets();
    CloseGameAudio();
    UnloadReadableFont();
    CloseWindow();
    return 0;
}

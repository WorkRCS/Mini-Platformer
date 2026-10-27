#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {
constexpr int VIEW_W = 960;
constexpr int VIEW_H = 540;
constexpr float WORLD_W = 2840.0f;
constexpr float PLAYER_W = 28.0f;
constexpr float PLAYER_H = 38.0f;
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

struct Platform { Rectangle box; };
struct Spike { Rectangle box; };
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
    Vector2 pos { 70.0f, 432.0f };
    Vector2 vel { 0.0f, 0.0f };
    bool grounded = false;
    int lives = 3;
};

Screen screen = Screen::Menu;
RenderTexture2D canvas {};
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

Player player;
int levelIndex = 0;
int difficulty = 0; // 0 Easy, 1 Normal, 2 Hard
int score = 0;
int highScore = 0;
float cameraX = 0.0f;
float gameClock = 0.0f;
float hurtFlash = 0.0f;
bool exitRequested = false;

const char* DifficultyName() {
    if (difficulty == 0) return "EASY";
    if (difficulty == 1) return "NORMAL";
    return "HARD";
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
    const int tw = MeasureText(label, fs);
    DrawText(label, static_cast<int>(r.x + (r.width - tw) * 0.5f),
             static_cast<int>(r.y + (r.height - fs) * 0.5f - 1),
             fs, active ? C_WHITE_90 : Color{ 133, 145, 165, 255 });
    return active && Pressed(r);
}

void TextCentered(const char* text, int y, int size, Color color) {
    DrawText(text, (VIEW_W - MeasureText(text, size)) / 2, y, size, color);
}

void SpawnParticles(Vector2 pos, Color color, int amount = 16) {
    for (int i = 0; i < amount; ++i) {
        const float a = (static_cast<float>(GetRandomValue(0, 359)) * PI) / 180.0f;
        const float s = static_cast<float>(GetRandomValue(55, 220));
        particles.push_back({ pos, { std::cos(a) * s, std::sin(a) * s },
                              GetRandomValue(35, 85) / 100.0f,
                              0.85f, static_cast<float>(GetRandomValue(2, 5)), color });
    }
}

void AddCoin(float x, float y, float phase) {
    coins.push_back({ { x, y }, false, phase });
}

void AddEnemy(float x, float y, float minX, float maxX, float baseSpeed) {
    float scale = difficulty == 0 ? 0.78f : (difficulty == 2 ? 1.35f : 1.0f);
    enemies.push_back({ { x, y, 30.0f, 27.0f }, minX, maxX,
                        baseSpeed * scale, 1, true });
}

void LoadLevel(int which) {
    levelIndex = std::clamp(which, 0, 2);
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
        spikes = { { { 248, 444, 52, 26 } }, { { 1080, 424, 54, 26 } },
                   { { 1915, 414, 50, 26 } }, { { 2490, 424, 50, 26 } } };
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
        spikes = { { { 176, 444, 55, 26 } }, { { 1075, 439, 54, 26 } },
                   { { 2070, 429, 50, 26 } }, { { 2620, 424, 52, 26 } } };
        AddCoin(270, 420, 0.0f); AddCoin(430, 380, 1.0f);
        AddCoin(630, 330, 2.0f); AddCoin(840, 375, 3.0f);
        AddCoin(1180, 415, 1.5f); AddCoin(1415, 365, 0.5f);
        AddCoin(1630, 310, 2.0f); AddCoin(1850, 360, 3.0f);
        AddCoin(2150, 405, 1.0f); AddCoin(2380, 350, 0.0f);
        AddCoin(2690, 415, 2.0f);
        AddEnemy(830, 393, 800, 950, 83.0f);
        AddEnemy(1830, 378, 1810, 1915, 94.0f);
        AddEnemy(2580, 423, 2560, 2790, 108.0f);
    } else {
        platforms = {
            { { 0, 470, 300, 100 } }, { { 355, 420, 130, 30 } },
            { { 530, 365, 130, 30 } }, { { 715, 410, 135, 30 } },
            { { 900, 350, 150, 30 } }, { { 1100, 455, 270, 100 } },
            { { 1420, 395, 135, 30 } }, { { 1600, 340, 145, 30 } },
            { { 1790, 395, 145, 30 } }, { { 1980, 455, 250, 100 } },
            { { 2285, 390, 145, 30 } }, { { 2480, 340, 145, 30 } },
            { { 2670, 450, 170, 100 } }
        };
        spikes = { { { 130, 444, 54, 26 } }, { { 1145, 429, 56, 26 } },
                   { { 2040, 429, 55, 26 } }, { { 2700, 424, 52, 26 } } };
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
    }

    player.pos = { 70.0f, 432.0f };
    player.vel = { 0.0f, 0.0f };
    player.grounded = false;
    cameraX = 0.0f;
    hurtFlash = 0.0f;
}

void StartGame() {
    score = 0;
    player.lives = StartingLives();
    LoadLevel(0);
    screen = Screen::Playing;
}

Rectangle PlayerRect() {
    return { player.pos.x, player.pos.y, PLAYER_W, PLAYER_H };
}

void TakeHit() {
    if (hurtFlash > 0.0f) return;
    --player.lives;
    hurtFlash = 1.1f;
    SpawnParticles({ player.pos.x + PLAYER_W * 0.5f,
                     player.pos.y + PLAYER_H * 0.5f }, C_CORAL, 22);
    player.pos = { 70.0f, 432.0f };
    player.vel = { 0.0f, 0.0f };
    player.grounded = false;
    cameraX = 0.0f;
    if (player.lives <= 0) screen = Screen::GameOver;
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
    gameClock += dt;
    hurtFlash = std::max(0.0f, hurtFlash - dt);
    UpdateParticles(dt);

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

    const float speed = difficulty == 0 ? 265.0f : (difficulty == 2 ? 325.0f : 295.0f);
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

    if (jumpPressed && player.grounded) {
        player.vel.y = -JUMP_SPEED;
        player.grounded = false;
    }

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

    for (auto& coin : coins) {
        if (coin.taken) continue;
        Rectangle c = { coin.center.x - 10.0f, coin.center.y - 10.0f, 20.0f, 20.0f };
        if (CheckCollisionRecs(PlayerRect(), c)) {
            coin.taken = true;
            score += 10;
            highScore = std::max(highScore, score);
            SpawnParticles(coin.center, C_GOLD, 10);
        }
    }

    for (const auto& s : spikes) {
        if (CheckCollisionRecs(PlayerRect(), s.box)) {
            TakeHit();
            break;
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
                SpawnParticles({ e.box.x + 15.0f, e.box.y + 10.0f }, C_CORAL, 15);
                score += 25;
                highScore = std::max(highScore, score);
            } else {
                TakeHit();
                break;
            }
        }
    }

    if (player.pos.y > VIEW_H + 110.0f && screen == Screen::Playing) TakeHit();

    const float targetCamera = std::clamp(player.pos.x - VIEW_W * 0.36f,
                                           0.0f, WORLD_W - VIEW_W);
    cameraX += (targetCamera - cameraX) * std::min(1.0f, dt * 5.0f);

    if (screen == Screen::Playing && player.pos.x > WORLD_W - 135.0f) {
        SpawnParticles({ WORLD_W - 100.0f, 385.0f }, C_MINT, 26);
        if (levelIndex < 2) {
            LoadLevel(levelIndex + 1);
        } else {
            screen = Screen::Victory;
        }
    }

    (void)previousX;
}

void DrawBackground(float camX) {
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 17, 28, 62, 255 },
                           Color{ 87, 158, 190, 255 });

    // Stars and distant glowing sky dots.
    for (int i = 0; i < 42; ++i) {
        const int x = (i * 137 + 41) % VIEW_W;
        const int y = (i * 71 + 23) % 260;
        const unsigned char alpha = static_cast<unsigned char>(110 + (i % 4) * 35);
        DrawCircle(x, y, (i % 7 == 0) ? 2.0f : 1.0f, Color{ 231, 243, 255, alpha });
    }
    DrawCircle(760, 105, 40, Color{ 255, 221, 148, 50 });
    DrawCircle(760, 105, 28, Color{ 255, 228, 165, 220 });

    // Soft, layered mountains create a parallax feel without external art.
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
    Rectangle r = { p.box.x - cameraX, p.box.y, p.box.width, p.box.height };
    if (r.x > VIEW_W || r.x + r.width < 0) return;
    DrawRectangleRounded({ r.x, r.y + 7, r.width, r.height }, 0.08f, 5,
                         Color{ 10, 23, 37, 110 });
    DrawRectangleRec(r, Color{ 42, 72, 93, 255 });
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y),
                  static_cast<int>(r.width), 8, Color{ 105, 222, 164, 255 });
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y + 8),
                  static_cast<int>(r.width), 3, Color{ 65, 155, 126, 255 });
    for (float x = r.x + 15; x < r.x + r.width - 8; x += 37) {
        DrawRectangle(static_cast<int>(x), static_cast<int>(r.y + 20),
                      13, 3, Color{ 70, 104, 126, 180 });
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
    const float x = spike.box.x - cameraX;
    if (x > VIEW_W || x + spike.box.width < 0) return;
    const int n = std::max(1, static_cast<int>(spike.box.width / 17.0f));
    const float sw = spike.box.width / n;
    for (int i = 0; i < n; ++i) {
        const float x1 = x + i * sw;
        DrawTriangle({ x1, spike.box.y + spike.box.height },
                     { x1 + sw * 0.5f, spike.box.y },
                     { x1 + sw, spike.box.y + spike.box.height },
                     Color{ 255, 106, 119, 255 });
        DrawTriangle({ x1 + sw * 0.5f, spike.box.y + 3.0f },
                     { x1 + sw * 0.5f, spike.box.y + 3.0f },
                     { x1 + sw * 0.82f, spike.box.y + spike.box.height - 1.0f },
                     Color{ 255, 175, 171, 255 });
    }
}

void DrawEnemy(const Enemy& e) {
    if (!e.alive) return;
    const float x = e.box.x - cameraX;
    if (x > VIEW_W || x + e.box.width < 0) return;
    DrawRectangleRounded({ x, e.box.y + 4, e.box.width, e.box.height - 4 },
                         0.35f, 6, Color{ 172, 72, 105, 255 });
    DrawRectangleRounded({ x + 3, e.box.y, e.box.width - 6, e.box.height - 7 },
                         0.35f, 6, C_CORAL);
    DrawCircle(static_cast<int>(x + 10), static_cast<int>(e.box.y + 10), 3.0f, WHITE);
    DrawCircle(static_cast<int>(x + 21), static_cast<int>(e.box.y + 10), 3.0f, WHITE);
    DrawCircle(static_cast<int>(x + 11 + e.direction), static_cast<int>(e.box.y + 10), 1.4f, C_INK);
    DrawCircle(static_cast<int>(x + 22 + e.direction), static_cast<int>(e.box.y + 10), 1.4f, C_INK);
}

void DrawPlayer() {
    const float x = player.pos.x - cameraX;
    const float bob = player.grounded && std::fabs(player.vel.x) > 1.0f
                      ? std::sin(gameClock * 17.0f) * 2.0f : 0.0f;
    const float y = player.pos.y + bob;
    if (hurtFlash > 0.0f && static_cast<int>(gameClock * 18.0f) % 2 == 0) return;

    DrawEllipse(static_cast<int>(x + PLAYER_W * 0.5f),
                static_cast<int>(player.pos.y + PLAYER_H + 2), 19, 5,
                Color{ 8, 21, 31, 75 });
    DrawRectangleRounded({ x + 1, y + 10, PLAYER_W - 2, PLAYER_H - 10 },
                         0.30f, 7, Color{ 67, 191, 173, 255 });
    DrawRectangleRounded({ x + 4, y + 4, PLAYER_W - 8, 21 },
                         0.45f, 8, Color{ 142, 255, 214, 255 });
    DrawRectangle(x + 6, y + 20, PLAYER_W - 12, 4, Color{ 49, 139, 145, 255 });
    DrawCircle(static_cast<int>(x + 11), static_cast<int>(y + 14), 3.1f, WHITE);
    DrawCircle(static_cast<int>(x + 21), static_cast<int>(y + 14), 3.1f, WHITE);
    const float look = player.vel.x > 0.0f ? 1.0f : (player.vel.x < 0.0f ? -1.0f : 0.0f);
    DrawCircle(static_cast<int>(x + 11 + look), static_cast<int>(y + 14), 1.5f, C_INK);
    DrawCircle(static_cast<int>(x + 21 + look), static_cast<int>(y + 14), 1.5f, C_INK);
    DrawRectangleRounded({ x + 5, y + PLAYER_H - 5, 7, 8 }, 0.3f, 4,
                         Color{ 36, 91, 116, 255 });
    DrawRectangleRounded({ x + 17, y + PLAYER_H - 5, 7, 8 }, 0.3f, 4,
                         Color{ 36, 91, 116, 255 });
}

void DrawGoal() {
    const float x = WORLD_W - 105.0f - cameraX;
    DrawRectangle(static_cast<int>(x), 360, 5, 90, Color{ 234, 244, 255, 240 });
    DrawTriangle({ x + 5, 360 }, { x + 63, 378 }, { x + 5, 397 }, C_MINT);
    DrawCircle(static_cast<int>(x + 2), 355, 6, C_GOLD);
    DrawText("GOAL", static_cast<int>(x - 12), 408, 12, C_WHITE_90);
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
    DrawText("JUMP", static_cast<int>(jump.x + 15), static_cast<int>(jump.y + 31),
             16, C_WHITE_90);
}

void DrawHud() {
    DrawRectangleRounded({ 16, 15, 288, 58 }, 0.18f, 8, Color{ 13, 26, 45, 205 });
    DrawText(TextFormat("LEVEL %d / 3", levelIndex + 1), 30, 24, 14,
             Color{ 159, 190, 220, 255 });
    DrawText(TextFormat("SCORE %04d", score), 30, 43, 19, C_GOLD);
    DrawText(TextFormat("LIVES %d", player.lives), 192, 43, 17,
             player.lives <= 2 ? C_CORAL : C_MINT);

    DrawRectangleRounded({ 768, 17, 174, 42 }, 0.20f, 7, Color{ 13, 26, 45, 205 });
    DrawText(DifficultyName(), 785, 29, 15, C_WHITE_90);
    if (Button({ 891, 20, 39, 36 }, "II")) screen = Screen::Paused;
    DrawText("ESC / P = PAUSE", 766, 65, 10, Color{ 225, 239, 250, 170 });
}

void DrawGame() {
    DrawBackground(cameraX);
    for (const auto& p : platforms) DrawPlatform(p);
    for (const auto& s : spikes) DrawSpike(s);
    for (const auto& c : coins) DrawCoin(c);
    for (const auto& e : enemies) DrawEnemy(e);
    DrawGoal();
    DrawParticles();
    DrawPlayer();
    DrawHud();
    DrawTouchControls();
}

void DrawTitle() {
    DrawBackground(0.0f);
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 10, 17, 38, 165 }, Color{ 13, 28, 48, 110 });
    DrawCircle(480, 126, 78, Color{ 105, 245, 190, 18 });
    DrawCircleLines(480, 126, 68, Color{ 129, 255, 210, 110 });
    DrawCircleLines(480, 126, 56, Color{ 129, 255, 210, 70 });
    TextCentered("TINY TRAILS", 82, 48, C_WHITE_90);
    TextCentered("A little platform adventure", 137, 18, C_MINT);
    DrawRectangleRounded({ 332, 183, 296, 333 }, 0.08f, 12,
                         Color{ 10, 22, 40, 205 });
    DrawRectangleRoundedLinesEx({ 332, 183, 296, 333 }, 0.08f, 12,
                              1.5f, Color{ 96, 135, 166, 160 });

    if (Button({ 370, 217, 220, 48 }, "PLAY")) StartGame();
    if (Button({ 370, 276, 220, 48 },
               TextFormat("DIFFICULTY: %s", DifficultyName()))) {
        difficulty = (difficulty + 1) % 3;
    }
    if (Button({ 370, 335, 220, 48 }, "HOW TO PLAY")) screen = Screen::Controls;
    DrawText("Keyboard  /  Gamepad  /  Touch", 354, 403, 16,
             Color{ 182, 204, 224, 255 });
    TextCentered("Collect coins  -  avoid spikes  -  reach the flag", 434, 14,
                 Color{ 166, 191, 214, 255 });
    TextCentered("3 short levels • no extra assets needed", 458, 13,
                 Color{ 129, 157, 183, 255 });
}

void DrawControlsScreen() {
    DrawBackground(0.0f);
    DrawRectangleGradientV(0, 0, VIEW_W, VIEW_H,
                           Color{ 10, 17, 38, 165 }, Color{ 13, 28, 48, 110 });
    TextCentered("TINY TRAILS", 76, 38, C_WHITE_90);
    DrawRectangleRounded({ 225, 150, 510, 290 }, 0.06f, 8, Color{ 12, 23, 43, 248 });
    TextCentered("HOW TO PLAY", 174, 28, C_MINT);
    DrawText("MOVE", 275, 226, 17, C_GOLD);
    DrawText("A / D or arrow keys", 425, 226, 17, C_WHITE_90);
    DrawText("JUMP", 275, 263, 17, C_GOLD);
    DrawText("Space / W / Up", 425, 263, 17, C_WHITE_90);
    DrawText("CONTROLLER", 275, 300, 17, C_GOLD);
    DrawText("D-pad or left stick + A button", 425, 300, 15, C_WHITE_90);
    DrawText("MOBILE", 275, 337, 17, C_GOLD);
    DrawText("Use the on-screen arrows and JUMP", 425, 337, 15, C_WHITE_90);
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
    DrawOverlay("YOU DID IT!", TextFormat("Trail complete  •  Score %d", score));
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
    canvas = LoadRenderTexture(VIEW_W, VIEW_H);
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose() && !exitRequested) {
        const float dt = std::min(GetFrameTime(), 0.033f);
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
        const Rectangle dest = { viewOffsetX, viewOffsetY,
                                 VIEW_W * viewScale, VIEW_H * viewScale };
        DrawTexturePro(canvas.texture, source, dest, { 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(canvas);
    CloseWindow();
    return 0;
}

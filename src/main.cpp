#include "raylib.h"
#include "raymath.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>

namespace {
constexpr int DESIGN_W = 960;
constexpr int DESIGN_H = 540;
constexpr float PLAYER_SPEED = 4.3f;
constexpr float PLAYER_RUN_SPEED = 6.2f;
constexpr float JUMP_SPEED = 6.2f;
constexpr float GRAVITY = 18.0f;
constexpr float PLAYER_SCALE = 1.0f;
constexpr float ENEMY_SCALE = 1.0f;
constexpr float BOSS_SCALE = 1.0f;
constexpr float WORLD_LIMIT = 32.0f;
constexpr unsigned int MAX_TOUCHES = 10;

const Color BG = { 13, 20, 29, 255 };
const Color PANEL = { 18, 29, 40, 235 };
const Color TEXT = { 230, 235, 237, 255 };
const Color MUTED = { 160, 177, 189, 255 };
const Color ACCENT = { 196, 173, 121, 255 };
const Color HEALTH = { 170, 73, 66, 255 };

enum class Screen { Menu, Intro, Playing, Paused, Complete };

struct AnimationClips {
    int idle = -1;
    int walk = -1;
    int run = -1;
    int jump = -1;
    int attack = -1;
    int hit = -1;
    int death = -1;
};

struct Actor {
    Model model = {};
    ModelAnimation* animations = nullptr;
    unsigned int animationCount = 0;
    AnimationClips clips = {};
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    Vector3 velocity = { 0.0f, 0.0f, 0.0f };
    float yaw = 0.0f;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float attackCooldown = 0.0f;
    float attackTimer = 0.0f;
    float hurtTimer = 0.0f;
    float animationClock = 0.0f;
    float deathTimer = 0.0f;
    int currentAnimation = -1;
    bool modelReady = false;
    bool active = false;
    bool alive = true;
    bool grounded = true;
    bool running = false;
};

struct TouchFrame {
    int count = 0;
    std::array<int, MAX_TOUCHES> ids = {};
    std::array<Vector2, MAX_TOUCHES> positions = {};
    std::array<Vector2, MAX_TOUCHES> deltas = {};
    std::array<Vector2, MAX_TOUCHES> newlyPressed = {};
    int newCount = 0;
};
TouchFrame touchFrame;
std::array<int, MAX_TOUCHES> previousIds = {};
std::array<Vector2, MAX_TOUCHES> previousPositions = {};
int previousCount = 0;

Actor player;
Actor grunt;
Actor boss;
Model environment = {};
bool environmentReady = false;
Music chapterMusic = {};
bool musicReady = false;
Screen screen = Screen::Menu;
Camera3D camera = {};
float cameraYaw = 0.0f;
float cameraPitch = 0.30f;
float cameraDistance = 5.4f;
float worldClock = 0.0f;
float playerHurtFlash = 0.0f;
int missionStep = 0;
bool chapterComplete = false;
std::string toast = "CHAPTER 01  |  THE SILENT TRAIL";
float toastTimer = 5.0f;

float UiScale() {
    return std::max(0.75f, std::min(GetScreenWidth()/960.0f, GetScreenHeight()/540.0f));
}
Rectangle JoystickRect() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float s = UiScale();
    return { 14.0f*s, sh - 194.0f*s, std::min(190.0f*s, sw * 0.30f), 178.0f*s };
}
Rectangle JumpButton() {
    const float s = std::max(0.75f, std::min(GetScreenWidth()/960.0f, GetScreenHeight()/540.0f));
    return { GetScreenWidth() - 220.0f*s, GetScreenHeight() - 106.0f*s, 88.0f*s, 70.0f*s };
}
Rectangle AttackButton() {
    const float s = std::max(0.75f, std::min(GetScreenWidth()/960.0f, GetScreenHeight()/540.0f));
    return { GetScreenWidth() - 112.0f*s, GetScreenHeight() - 116.0f*s, 96.0f*s, 96.0f*s };
}
Rectangle InteractButton() {
    const float s = std::max(0.75f, std::min(GetScreenWidth()/960.0f, GetScreenHeight()/540.0f));
    return { GetScreenWidth() - 118.0f*s, GetScreenHeight() - 226.0f*s, 88.0f*s, 60.0f*s };
}

void CaptureTouchFrame() {
    touchFrame = {};
    touchFrame.count = std::min(GetTouchPointCount(), static_cast<int>(MAX_TOUCHES));
    for (int i = 0; i < touchFrame.count; ++i) {
        const int id = GetTouchPointId(i);
        const Vector2 pos = GetTouchPosition(i);
        touchFrame.ids[static_cast<size_t>(i)] = id;
        touchFrame.positions[static_cast<size_t>(i)] = pos;

        bool found = false;
        for (int j = 0; j < previousCount; ++j) {
            if (previousIds[static_cast<size_t>(j)] == id) {
                touchFrame.deltas[static_cast<size_t>(i)] = {
                    pos.x - previousPositions[static_cast<size_t>(j)].x,
                    pos.y - previousPositions[static_cast<size_t>(j)].y
                };
                found = true;
                break;
            }
        }
        if (!found && touchFrame.newCount < static_cast<int>(MAX_TOUCHES)) {
            touchFrame.newlyPressed[static_cast<size_t>(touchFrame.newCount++)] = pos;
        }
    }
    previousCount = touchFrame.count;
    for (int i = 0; i < touchFrame.count; ++i) {
        previousIds[static_cast<size_t>(i)] = touchFrame.ids[static_cast<size_t>(i)];
        previousPositions[static_cast<size_t>(i)] = touchFrame.positions[static_cast<size_t>(i)];
    }
}

bool TouchPressed(Rectangle rect) {
    for (int i = 0; i < touchFrame.newCount; ++i) {
        if (CheckCollisionPointRec(touchFrame.newlyPressed[static_cast<size_t>(i)], rect)) return true;
    }
    return IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(GetMousePosition(), rect);
}

bool TouchHeld(Rectangle rect) {
    for (int i = 0; i < touchFrame.count; ++i) {
        if (CheckCollisionPointRec(touchFrame.positions[static_cast<size_t>(i)], rect)) return true;
    }
    return IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
           CheckCollisionPointRec(GetMousePosition(), rect);
}

std::string Lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

int FindClip(const Actor& actor, const char* keyword) {
    const std::string wanted = Lower(keyword);
    for (unsigned int i = 0; i < actor.animationCount; ++i) {
        if (Lower(actor.animations[i].name).find(wanted) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Actor LoadActor(const char* path, Vector3 spawn, float health) {
    Actor actor = {};
    actor.position = spawn;
    actor.health = health;
    actor.maxHealth = health;

    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "ASSET MISSING: %s", path);
        return actor;
    }

    actor.model = LoadModel(path);
    actor.modelReady = actor.model.meshCount > 0;
    if (!actor.modelReady) {
        TraceLog(LOG_WARNING, "MODEL FAILED: %s", path);
        return actor;
    }

    actor.animations = LoadModelAnimations(path, &actor.animationCount);
    if (actor.animations != nullptr && actor.animationCount > 0) {
        actor.clips.idle = FindClip(actor, "idle");
        actor.clips.walk = FindClip(actor, "walk");
        actor.clips.run = FindClip(actor, "run");
        actor.clips.jump = FindClip(actor, "jump");
        actor.clips.attack = FindClip(actor, "attack");
        actor.clips.hit = FindClip(actor, "hit");
        actor.clips.death = FindClip(actor, "death");
    }

    if (actor.clips.walk < 0) actor.clips.walk = actor.clips.run;
    if (actor.clips.run < 0) actor.clips.run = actor.clips.walk;
    if (actor.clips.jump < 0) actor.clips.jump = actor.clips.idle;
    if (actor.clips.hit < 0) actor.clips.hit = actor.clips.idle;
    if (actor.clips.death < 0) actor.clips.death = actor.clips.idle;

    TraceLog(LOG_INFO, "Loaded %s: %u animation clips", path, actor.animationCount);
    return actor;
}

void UnloadActor(Actor& actor) {
    if (actor.animations != nullptr) UnloadModelAnimations(actor.animations, actor.animationCount);
    if (actor.modelReady) UnloadModel(actor.model);
    actor = {};
}

void SetToast(const char* message, float seconds = 3.0f) {
    toast = message;
    toastTimer = seconds;
}

void SelectAndApplyAnimation(Actor& actor, int clip, float dt, bool loop = true) {
    if (!actor.modelReady || actor.animations == nullptr || actor.animationCount == 0 ||
        clip < 0 || static_cast<unsigned int>(clip) >= actor.animationCount) return;

    if (actor.currentAnimation != clip) {
        actor.currentAnimation = clip;
        actor.animationClock = 0.0f;
    }
    actor.animationClock += dt;
    const int frames = std::max(1, actor.animations[static_cast<unsigned int>(clip)].frameCount);
    int frame = static_cast<int>(actor.animationClock * 24.0f);
    frame = loop ? frame % frames : std::min(frame, frames - 1);
    UpdateModelAnimation(actor.model, actor.animations[static_cast<unsigned int>(clip)], frame);
}

int CurrentClip(const Actor& actor, bool moving, bool running, bool jumping, bool attacking, bool hurt) {
    if (hurt && actor.clips.hit >= 0) return actor.clips.hit;
    if (attacking && actor.clips.attack >= 0) return actor.clips.attack;
    if (jumping && actor.clips.jump >= 0) return actor.clips.jump;
    if (running && actor.clips.run >= 0) return actor.clips.run;
    if (moving && actor.clips.walk >= 0) return actor.clips.walk;
    return actor.clips.idle;
}

bool RequiredPlayerClipsReady() {
    return player.modelReady && player.animationCount > 0 &&
           player.clips.idle >= 0 && player.clips.walk >= 0 &&
           player.clips.run >= 0 && player.clips.jump >= 0 &&
           player.clips.attack >= 0 && player.clips.hit >= 0 &&
           player.clips.death >= 0;
}

bool RequiredCombatClipsReady(const Actor& actor) {
    return actor.modelReady && actor.animationCount > 0 &&
           actor.clips.idle >= 0 && (actor.clips.walk >= 0 || actor.clips.run >= 0) &&
           actor.clips.attack >= 0 && actor.clips.hit >= 0 && actor.clips.death >= 0;
}

bool AllCoreAssetsReady() {
    return environmentReady && RequiredPlayerClipsReady() &&
           RequiredCombatClipsReady(grunt) && RequiredCombatClipsReady(boss);
}

void LoadGameAssets() {
    // Android APK paths are relative to the packaged assets root.
    player = LoadActor("characters/player.glb", { 0.0f, 0.0f, 0.0f }, 100.0f);
    grunt = LoadActor("characters/enemy_grunt.glb", { 4.0f, 0.0f, 12.0f }, 65.0f);
    boss = LoadActor("characters/boss_chapter_01.glb", { 0.0f, 0.0f, 20.0f }, 180.0f);

    if (FileExists("environment/chapter_01.glb")) {
        environment = LoadModel("environment/chapter_01.glb");
        environmentReady = environment.meshCount > 0;
    } else {
        TraceLog(LOG_WARNING, "ASSET MISSING: environment/chapter_01.glb");
    }
    if (FileExists("audio/chapter_01.ogg")) {
        chapterMusic = LoadMusicStream("audio/chapter_01.ogg");
        musicReady = true;
        chapterMusic.looping = true;
    }

    grunt.active = true;
    boss.active = false;
}

void ResetChapter() {
    player.position = { 0.0f, 0.0f, 0.0f };
    player.velocity = { 0.0f, 0.0f, 0.0f };
    player.yaw = 0.0f;
    player.health = player.maxHealth;
    player.alive = true;
    player.grounded = true;
    player.attackCooldown = 0.0f;
    player.attackTimer = 0.0f;
    player.currentAnimation = -1;

    grunt.position = { 4.0f, 0.0f, 12.0f };
    grunt.health = grunt.maxHealth;
    grunt.active = true;
    grunt.alive = true;
    grunt.currentAnimation = -1;
    grunt.attackCooldown = 0.4f;

    boss.position = { 0.0f, 0.0f, 20.0f };
    boss.health = boss.maxHealth;
    boss.active = false;
    boss.alive = true;
    boss.currentAnimation = -1;
    boss.attackCooldown = 1.1f;

    missionStep = 0;
    chapterComplete = false;
    playerHurtFlash = 0.0f;
    cameraYaw = 0.0f;
    cameraPitch = 0.30f;
    cameraDistance = 5.4f;
    SetToast("CHAPTER 01  |  THE SILENT TRAIL", 5.0f);
}

Vector3 ForwardFromYaw(float yaw) {
    return { std::sin(yaw), 0.0f, std::cos(yaw) };
}

void UpdateCamera(float dt) {
    const Vector3 forward = ForwardFromYaw(cameraYaw);
    const Vector3 focus = { player.position.x, player.position.y + 1.15f, player.position.z };
    const float horizontal = cameraDistance * std::cos(cameraPitch);
    const Vector3 desired = {
        focus.x - forward.x * horizontal,
        focus.y + cameraDistance * std::sin(cameraPitch) + 0.35f,
        focus.z - forward.z * horizontal
    };
    camera.position = Vector3Lerp(camera.position, desired, Clamp(dt * 7.0f, 0.0f, 1.0f));
    camera.target = Vector3Lerp(camera.target, focus, Clamp(dt * 9.0f, 0.0f, 1.0f));
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 50.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

void ApplyCameraTouch() {
    for (int i = 0; i < touchFrame.count; ++i) {
        const Vector2 p = touchFrame.positions[static_cast<size_t>(i)];
        if (p.x < GetScreenWidth() * 0.42f) continue;
        if (CheckCollisionPointRec(p, AttackButton()) ||
            CheckCollisionPointRec(p, JumpButton()) ||
            CheckCollisionPointRec(p, InteractButton())) continue;

        const Vector2 delta = touchFrame.deltas[static_cast<size_t>(i)];
        cameraYaw -= delta.x * 0.006f;
        cameraPitch = Clamp(cameraPitch + delta.y * 0.0038f, 0.12f, 0.68f);
    }
}

void HurtPlayer(float amount) {
    if (player.hurtTimer > 0.0f || !player.alive) return;
    player.health = std::max(0.0f, player.health - amount);
    player.hurtTimer = 0.55f;
    playerHurtFlash = 0.28f;
    if (player.health <= 0.0f) {
        player.alive = false;
        player.deathTimer = 1.6f;
        player.currentAnimation = -1;
        SetToast("YOU FELL  |  TAP TO RESTART", 30.0f);
    }
}

void UpdateEnemy(Actor& actor, float dt, float noticeRange, float damage) {
    if (!actor.active || !actor.modelReady) return;
    if (!actor.alive) {
        if (actor.deathTimer > 0.0f) {
            actor.deathTimer = std::max(0.0f, actor.deathTimer - dt);
            SelectAndApplyAnimation(actor, actor.clips.death, dt, false);
        }
        return;
    }
    actor.attackCooldown = std::max(0.0f, actor.attackCooldown - dt);
    actor.attackTimer = std::max(0.0f, actor.attackTimer - dt);
    actor.hurtTimer = std::max(0.0f, actor.hurtTimer - dt);

    Vector3 delta = Vector3Subtract(player.position, actor.position);
    delta.y = 0.0f;
    const float distance = Vector3Length(delta);
    bool moving = false;
    if (distance < noticeRange && distance > 1.8f && player.alive) {
        const Vector3 direction = Vector3Scale(delta, 1.0f / std::max(distance, 0.001f));
        actor.position = Vector3Add(actor.position, Vector3Scale(direction, (actor.health > 90.0f ? 2.25f : 2.0f) * dt));
        actor.yaw = std::atan2(direction.x, direction.z);
        moving = true;
    } else if (distance <= 1.8f && actor.attackCooldown <= 0.0f && player.alive) {
        actor.attackCooldown = 1.5f;
        actor.attackTimer = 0.48f;
        HurtPlayer(damage);
    }

    actor.position.x = Clamp(actor.position.x, -WORLD_LIMIT, WORLD_LIMIT);
    actor.position.z = Clamp(actor.position.z, -WORLD_LIMIT, WORLD_LIMIT);
    const int clip = CurrentClip(actor, moving, moving, false, actor.attackTimer > 0.0f,
                                 actor.hurtTimer > 0.0f);
    SelectAndApplyAnimation(actor, clip, dt);
}

void AttackTarget(Actor& target, float damage) {
    if (!target.active || !target.alive || !target.modelReady) return;
    const float distance = Vector3Distance(
        { player.position.x, 0.0f, player.position.z },
        { target.position.x, 0.0f, target.position.z });
    if (distance > 2.7f) return;

    Vector3 toTarget = Vector3Subtract(target.position, player.position);
    toTarget.y = 0.0f;
    toTarget = Vector3Normalize(toTarget);
    const Vector3 facing = ForwardFromYaw(player.yaw);
    if (Vector3DotProduct(toTarget, facing) < -0.2f) return;

    target.health = std::max(0.0f, target.health - damage);
    target.hurtTimer = 0.12f;
    if (target.health <= 0.0f) {
        target.alive = false;
        target.deathTimer = 1.6f;
        target.currentAnimation = -1;
        if (&target == &grunt) {
            missionStep = 2;
            boss.active = true;
            SetToast("THE SENTINEL HAS AWAKENED", 4.0f);
        } else {
            screen = Screen::Complete;
            chapterComplete = true;
            SetToast("CHAPTER COMPLETE", 10.0f);
        }
    }
}

void UpdatePlayer(float dt) {
    player.attackCooldown = std::max(0.0f, player.attackCooldown - dt);
    player.attackTimer = std::max(0.0f, player.attackTimer - dt);
    player.hurtTimer = std::max(0.0f, player.hurtTimer - dt);
    playerHurtFlash = std::max(0.0f, playerHurtFlash - dt);
    if (!player.alive) {
        if (player.deathTimer > 0.0f) {
            player.deathTimer = std::max(0.0f, player.deathTimer - dt);
            SelectAndApplyAnimation(player, player.clips.death, dt, false);
        }
        if (TouchPressed({ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() })) {
            ResetChapter();
            screen = Screen::Playing;
        }
        return;
    }

    ApplyCameraTouch();
    Vector3 input = { 0.0f, 0.0f, 0.0f };
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.x += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.x -= 1.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.z += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.z -= 1.0f;

    const Rectangle joy = JoystickRect();
    for (int i = 0; i < touchFrame.count; ++i) {
        const Vector2 p = touchFrame.positions[static_cast<size_t>(i)];
        if (!CheckCollisionPointRec(p, joy)) continue;
        const Vector2 center = { joy.x + joy.width * 0.5f, joy.y + joy.height * 0.5f };
        input.x += Clamp((p.x - center.x) / (joy.width * 0.42f), -1.0f, 1.0f);
        input.z += Clamp((center.y - p.y) / (joy.height * 0.42f), -1.0f, 1.0f);
    }

    const float inputLength = std::sqrt(input.x * input.x + input.z * input.z);
    if (inputLength > 1.0f) {
        input.x /= inputLength;
        input.z /= inputLength;
    }

    const float sy = std::sin(cameraYaw);
    const float cy = std::cos(cameraYaw);
    Vector3 movement = {
        input.x * cy + input.z * sy,
        0.0f,
        -input.x * sy + input.z * cy
    };
    const float moveLength = Vector3Length(movement);
    const bool moving = moveLength > 0.12f;
    player.running = moving && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                                (inputLength > 0.92f && touchFrame.count > 0 && input.z > 0.8f));
    if (moving) {
        movement = Vector3Scale(movement, 1.0f / moveLength);
        player.yaw = std::atan2(movement.x, movement.z);
        const float speed = player.running ? PLAYER_RUN_SPEED : PLAYER_SPEED;
        player.position.x += movement.x * speed * dt;
        player.position.z += movement.z * speed * dt;
    }

    const bool jump = IsKeyPressed(KEY_SPACE) || TouchPressed(JumpButton());
    if (jump && player.grounded) {
        player.velocity.y = JUMP_SPEED;
        player.grounded = false;
    }
    player.velocity.y -= GRAVITY * dt;
    player.position.y += player.velocity.y * dt;
    if (player.position.y <= 0.0f) {
        player.position.y = 0.0f;
        player.velocity.y = 0.0f;
        player.grounded = true;
    }

    player.position.x = Clamp(player.position.x, -WORLD_LIMIT, WORLD_LIMIT);
    player.position.z = Clamp(player.position.z, -WORLD_LIMIT, WORLD_LIMIT);

    const bool attack = IsKeyPressed(KEY_J) || TouchPressed(AttackButton());
    if (attack && player.attackCooldown <= 0.0f) {
        player.attackCooldown = 0.48f;
        player.attackTimer = 0.38f;
        if (missionStep >= 1) AttackTarget(grunt, 32.0f);
        if (missionStep >= 2) AttackTarget(boss, 30.0f);
    }

    const bool interact = IsKeyPressed(KEY_E) || TouchPressed(InteractButton());
    if (interact && missionStep == 0) {
        const float gateDistance = Vector3Distance(
            { player.position.x, 0.0f, player.position.z }, { 0.0f, 0.0f, 8.0f });
        if (gateDistance < 3.2f) {
            missionStep = 1;
            SetToast("A SHADOW BLOCKS THE TRAIL", 3.5f);
        } else {
            SetToast("FOLLOW THE TRAIL TO THE OLD GATE", 2.0f);
        }
    }

    const int playerClip = CurrentClip(player, moving, player.running, !player.grounded,
                                       player.attackTimer > 0.0f, player.hurtTimer > 0.0f);
    SelectAndApplyAnimation(player, playerClip, dt);

    UpdateEnemy(grunt, dt, missionStep >= 1 ? 24.0f : 0.0f, 8.0f);
    UpdateEnemy(boss, dt, missionStep >= 2 ? 30.0f : 0.0f, 14.0f);
    UpdateCamera(dt);
}

void DrawActor(const Actor& actor, float scale) {
    if (!actor.modelReady) return;
    if (actor.alive || actor.clips.death < 0) {
        DrawModelEx(actor.model, actor.position, { 0.0f, 1.0f, 0.0f },
                    actor.yaw * RAD2DEG, { scale, scale, scale },
                    actor.hurtTimer > 0.0f ? Color{ 255, 224, 218, 255 } : WHITE);
    } else {
        DrawModelEx(actor.model, actor.position, { 0.0f, 1.0f, 0.0f },
                    actor.yaw * RAD2DEG, { scale, scale, scale }, WHITE);
    }
}

void DrawHealthBar(const Actor& actor, Color fill) {
    if (!actor.active || !actor.alive) return;
    const float ratio = Clamp(actor.health / std::max(1.0f, actor.maxHealth), 0.0f, 1.0f);
    const Vector2 pos = GetWorldToScreen({ actor.position.x, actor.position.y + 2.2f, actor.position.z }, camera);
    const float width = 72.0f;
    DrawRectangleRounded({ pos.x - width / 2, pos.y, width, 7 }, 0.4f, 6, Color{ 18, 23, 28, 210 });
    DrawRectangleRounded({ pos.x - width / 2 + 1, pos.y + 1, (width - 2) * ratio, 5 }, 0.4f, 6, fill);
}

void DrawButton(Rectangle rect, const char* label, Color color) {
    DrawRectangleRounded(rect, 0.24f, 8, Fade(color, 0.78f));
    DrawRectangleRoundedLinesEx(rect, 0.24f, 8, 1.5f, Fade(WHITE, 0.78f));
    const int size = static_cast<int>(15*UiScale());
    const int width = MeasureText(label, size);
    DrawText(label, static_cast<int>(rect.x + (rect.width - width) * 0.5f),
             static_cast<int>(rect.y + (rect.height - size) * 0.5f), size, WHITE);
}

void DrawHud() {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    DrawRectangleRounded({ 16, 14, 245, 69 }, 0.08f, 8, PANEL);
    DrawText("WILDBOUND", 29, 23, 19, ACCENT);
    DrawText(TextFormat("HEALTH  %i / %i", static_cast<int>(player.health),
                        static_cast<int>(player.maxHealth)), 29, 48, 12, TEXT);
    DrawRectangleRounded({ 29, 66, 215, 8 }, 0.35f, 6, Color{ 49, 56, 62, 255 });
    DrawRectangleRounded({ 30, 67, 213.0f * (player.health / player.maxHealth), 6 },
                         0.35f, 6, player.health < 30.0f ? HEALTH : Color{ 102, 158, 120, 255 });

    const char* mission = missionStep == 0 ? "OBJECTIVE  |  REACH THE OLD GATE"
                         : missionStep == 1 ? "OBJECTIVE  |  DEFEAT THE GRUNT"
                         : "OBJECTIVE  |  DEFEAT THE SENTINEL";
    const int tw = MeasureText(mission, 14);
    DrawRectangleRounded({ (sw - tw - 28.0f) / 2, 17, tw + 28.0f, 31 }, 0.22f, 8, PANEL);
    DrawText(mission, (sw - tw) / 2, 26, 14, TEXT);

    DrawCircle(static_cast<int>(JoystickRect().x + JoystickRect().width * 0.5f),
               static_cast<int>(JoystickRect().y + JoystickRect().height * 0.5f),
               JoystickRect().width * 0.43f, Fade(BG, 0.45f));
    DrawCircleLines(static_cast<int>(JoystickRect().x + JoystickRect().width * 0.5f),
                    static_cast<int>(JoystickRect().y + JoystickRect().height * 0.5f),
                    JoystickRect().width * 0.43f, Fade(ACCENT, 0.75f));
    const float s = UiScale();
    DrawText("MOVE", static_cast<int>(JoystickRect().x + 58*s),
             static_cast<int>(JoystickRect().y + 76*s), static_cast<int>(13*s), TEXT);

    DrawButton(JumpButton(), "JUMP", Color{ 67, 113, 135, 255 });
    DrawButton(AttackButton(), "ATTACK", Color{ 142, 70, 63, 255 });
    DrawButton(InteractButton(), "USE", Color{ 117, 105, 77, 255 });

    if (boss.active && boss.alive) {
        const char* name = "THE SENTINEL";
        const int nameWidth = MeasureText(name, 15);
        DrawText(name, (sw - nameWidth) / 2, 58, 15, ACCENT);
        DrawRectangleRounded({ sw * 0.32f, 80, sw * 0.36f, 10 }, 0.3f, 6, Color{ 42, 45, 49, 255 });
        DrawRectangleRounded({ sw * 0.32f + 1, 81,
                               (sw * 0.36f - 2) * (boss.health / boss.maxHealth), 8 },
                             0.3f, 6, HEALTH);
    }

    if (toastTimer > 0.0f) {
        const int width = MeasureText(toast.c_str(), 14);
        DrawRectangleRounded({ (sw - width - 28.0f) / 2, sh * 0.18f, width + 28.0f, 34 },
                             0.18f, 8, Fade(PANEL, 0.92f));
        DrawText(toast.c_str(), (sw - width) / 2, static_cast<int>(sh * 0.18f + 10),
                 14, ACCENT);
    }
    if (playerHurtFlash > 0.0f) DrawRectangle(0, 0, sw, sh, Fade(HEALTH, playerHurtFlash * 0.28f));
}

void DrawMenu() {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Fade(BG, 0.62f));
    DrawText("WILDBOUND", static_cast<int>(sw * 0.10f), static_cast<int>(sh * 0.22f), 48, ACCENT);
    DrawText("CHAPTER 01  |  THE SILENT TRAIL",
             static_cast<int>(sw * 0.105f), static_cast<int>(sh * 0.35f), 18, TEXT);
    DrawText("A quiet trail. A sealed gate. Something is waiting beyond it.",
             static_cast<int>(sw * 0.105f), static_cast<int>(sh * 0.42f), 13, MUTED);

    const bool ready = AllCoreAssetsReady();
    const Rectangle start = { sw * 0.105f, sh * 0.56f, sw * 0.30f, 54 };
    DrawRectangleRounded(start, 0.16f, 8, ready ? ACCENT : Color{ 66, 72, 76, 255 });
    DrawText("START CHAPTER", static_cast<int>(start.x + 20),
             static_cast<int>(start.y + 17), 17, BG);
    DrawText("Offline story prototype  |  Touch to move, swipe right to orbit",
             static_cast<int>(sw * 0.105f), static_cast<int>(sh * 0.75f), 12, MUTED);

    if (!ready) {
        const float panelY = sh * 0.53f;
        DrawRectangleRounded({ sw * 0.48f, panelY, sw * 0.46f, sh * 0.34f }, 0.06f, 8, PANEL);
        DrawText("ASSET SETUP REQUIRED", static_cast<int>(sw * 0.51f),
                 static_cast<int>(panelY + 13), 16, ACCENT);
        DrawText(FileExists("characters/player.glb") ? "Player model found" : "Missing: characters/player.glb",
                 static_cast<int>(sw * 0.51f), static_cast<int>(panelY + 42), 12,
                 FileExists("characters/player.glb") ? TEXT : HEALTH);
        DrawText(environmentReady ? "Chapter environment found" : "Missing: environment/chapter_01.glb",
                 static_cast<int>(sw * 0.51f), static_cast<int>(panelY + 63), 12,
                 environmentReady ? TEXT : HEALTH);
        DrawText(grunt.modelReady ? "Grunt model found" : "Missing: characters/enemy_grunt.glb",
                 static_cast<int>(sw * 0.51f), static_cast<int>(panelY + 84), 12,
                 grunt.modelReady ? TEXT : HEALTH);
        DrawText(boss.modelReady ? "Boss model found" : "Missing: characters/boss_chapter_01.glb",
                 static_cast<int>(sw * 0.51f), static_cast<int>(panelY + 105), 12,
                 boss.modelReady ? TEXT : HEALTH);
        DrawText(RequiredPlayerClipsReady() ? "Player locomotion clips found"
                                            : "Check Player: Idle/Walk/Run/Jump/Attack clips",
                 static_cast<int>(sw * 0.51f), static_cast<int>(panelY + 126), 11,
                 RequiredPlayerClipsReady() ? TEXT : HEALTH);
    }
}

void DrawOverlay(const char* title, const char* subtitle, const char* action) {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Fade(BG, 0.80f));
    const int titleWidth = MeasureText(title, 36);
    DrawText(title, (sw - titleWidth) / 2, static_cast<int>(sh * 0.34f), 36, ACCENT);
    const int subWidth = MeasureText(subtitle, 15);
    DrawText(subtitle, (sw - subWidth) / 2, static_cast<int>(sh * 0.47f), 15, TEXT);
    const int actionWidth = MeasureText(action, 13);
    DrawText(action, (sw - actionWidth) / 2, static_cast<int>(sh * 0.60f), 13, MUTED);
}

} // namespace

int main() {
#if defined(__ANDROID__) || defined(PLATFORM_ANDROID)
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(DESIGN_W, DESIGN_H, "Wildbound: Story");
#else
    InitWindow(DESIGN_W, DESIGN_H, "Wildbound: Story — visual asset test");
#endif
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    camera = {};
    camera.position = { 0.0f, 3.0f, -6.0f };
    camera.target = { 0.0f, 1.0f, 0.0f };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 50.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    LoadGameAssets();
    ResetChapter();
    if (musicReady) PlayMusicStream(chapterMusic);

    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.033f);
        worldClock += dt;
        if (toastTimer > 0.0f) toastTimer = std::max(0.0f, toastTimer - dt);
        CaptureTouchFrame();

        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();

        if (screen == Screen::Menu) {
            if (AllCoreAssetsReady() &&
                (IsKeyPressed(KEY_ENTER) || TouchPressed({ sw * 0.09f, sh * 0.50f, sw * 0.36f, sh * 0.18f }))) {
                screen = Screen::Intro;
            }
        } else if (screen == Screen::Intro) {
            if (IsKeyPressed(KEY_ENTER) || TouchPressed({ 0, 0, (float)sw, (float)sh })) {
                ResetChapter();
                screen = Screen::Playing;
                SetToast("FIND THE OLD GATE", 3.5f);
            }
        } else if (screen == Screen::Playing) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) ||
                TouchPressed({ sw - 64.0f, 12.0f, 52.0f, 44.0f })) {
                screen = Screen::Paused;
            } else {
                UpdatePlayer(dt);
            }
        } else if (screen == Screen::Paused) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) ||
                IsKeyPressed(KEY_ENTER) || TouchPressed({ 0, 0, (float)sw, (float)sh })) {
                screen = Screen::Playing;
            }
        } else if (screen == Screen::Complete) {
            if (IsKeyPressed(KEY_ENTER) || TouchPressed({ 0, 0, (float)sw, (float)sh })) {
                ResetChapter();
                screen = Screen::Menu;
            }
        }
        if (musicReady) UpdateMusicStream(chapterMusic);

        BeginDrawing();
        ClearBackground(BG);
        if (screen != Screen::Menu && environmentReady) {
            BeginMode3D(camera);
            DrawModel(environment, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
            DrawActor(player, PLAYER_SCALE);
            if (grunt.active && grunt.alive) DrawActor(grunt, ENEMY_SCALE);
            if (boss.active && boss.alive) DrawActor(boss, BOSS_SCALE);
            EndMode3D();
            DrawHealthBar(grunt, HEALTH);
            DrawHealthBar(boss, ACCENT);
        } else if (screen == Screen::Menu && environmentReady) {
            BeginMode3D(camera);
            DrawModel(environment, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);
            DrawActor(player, PLAYER_SCALE);
            EndMode3D();
        }

        if (screen == Screen::Menu) DrawMenu();
        else if (screen == Screen::Intro) DrawOverlay(
            "THE SILENT TRAIL",
            "Find the old gate. Defeat what has trapped the trail.",
            "TAP TO BEGIN");
        else if (screen == Screen::Playing) {
            DrawHud();
            DrawText("II", sw - 51, 22, 18, TEXT);
        } else if (screen == Screen::Paused) {
            DrawOverlay("PAUSED", "The trail can wait.", "TAP TO RESUME");
        } else if (screen == Screen::Complete) {
            DrawOverlay("CHAPTER COMPLETE", "The Sentinel has fallen. The trail is open again.", "TAP TO RESTART");
        }
        EndDrawing();
    }

    if (musicReady) {
        StopMusicStream(chapterMusic);
        UnloadMusicStream(chapterMusic);
    }
    if (environmentReady) UnloadModel(environment);
    UnloadActor(player);
    UnloadActor(grunt);
    UnloadActor(boss);
    CloseWindow();
    return 0;
}

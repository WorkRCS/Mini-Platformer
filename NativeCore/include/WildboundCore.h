#pragma once
#if defined(_WIN32)
  #if defined(WILDBOUND_CORE_EXPORTS)
    #define WB_API __declspec(dllexport)
  #else
    #define WB_API __declspec(dllimport)
  #endif
#else
  #define WB_API __attribute__((visibility("default")))
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct WB_ActorState {
    float x, y, z, yawDegrees, health, maxHealth;
    int alive, animationState, kind;
} WB_ActorState;
WB_API int wb_core_version(void);
WB_API void wb_reset(void);
WB_API void wb_update(float dt, float moveX, float moveZ, int jumpPressed, int attackPressed, int abilityPressed, int sprint);
WB_API void wb_get_player(WB_ActorState* outState);
WB_API int wb_get_monster_count(void);
WB_API void wb_get_monster(int index, WB_ActorState* outState);
WB_API int wb_get_boss_count(void);
WB_API void wb_get_boss(int index, WB_ActorState* outState);
WB_API int wb_get_kills(void);
WB_API int wb_get_xp(void);
WB_API int wb_get_defeated_bosses(void);
WB_API int wb_get_active_boss(void);
#ifdef __cplusplus
}
#endif

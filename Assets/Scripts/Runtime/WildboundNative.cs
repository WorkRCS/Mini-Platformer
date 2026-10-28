using System;
using System.Runtime.InteropServices;
using UnityEngine;

internal static class WildboundNative
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct ActorState
    {
        public float x, y, z, yawDegrees, health, maxHealth;
        public int alive, animationState, kind;
        public Vector3 Position { get { return new Vector3(x, y, z); } }
    }

    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern int wb_core_version();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern void wb_reset();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern void wb_update(float dt, float moveX, float moveZ, int jump, int attack, int ability, int sprint);
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern void wb_get_player(out ActorState state);
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern int wb_get_monster_count();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern void wb_get_monster(int index, out ActorState state);
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern int wb_get_boss_count();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] private static extern void wb_get_boss(int index, out ActorState state);
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] internal static extern int wb_get_kills();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] internal static extern int wb_get_xp();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] internal static extern int wb_get_defeated_bosses();
    [DllImport("WildboundCore", CallingConvention = CallingConvention.Cdecl)] internal static extern int wb_get_active_boss();

    private static bool available;

    internal static bool TryInitialize(out string message)
    {
        try
        {
            if (wb_core_version() < 1) throw new EntryPointNotFoundException("Unsupported native core version.");
            wb_reset();
            available = true;
            message = "C++ gameplay core loaded.";
            return true;
        }
        catch (Exception ex) when (ex is DllNotFoundException || ex is EntryPointNotFoundException || ex is BadImageFormatException)
        {
            available = false;
            message = "C++ native plug-in is not built for this editor/platform yet: " + ex.Message;
            return false;
        }
    }

    internal static bool IsAvailable { get { return available; } }
    internal static void Reset() { if (available) wb_reset(); }
    internal static void Update(float dt, Vector3 move, bool jump, bool attack, bool ability, bool sprint)
    {
        if (available) wb_update(dt, move.x, move.z, jump ? 1 : 0, attack ? 1 : 0, ability ? 1 : 0, sprint ? 1 : 0);
    }
    internal static ActorState GetPlayer() { ActorState s = new ActorState(); if (available) wb_get_player(out s); return s; }
    internal static int MonsterCount { get { return available ? wb_get_monster_count() : 0; } }
    internal static ActorState GetMonster(int i) { ActorState s = new ActorState(); if (available) wb_get_monster(i, out s); return s; }
    internal static int BossCount { get { return available ? wb_get_boss_count() : 0; } }
    internal static ActorState GetBoss(int i) { ActorState s = new ActorState(); if (available) wb_get_boss(i, out s); return s; }
}

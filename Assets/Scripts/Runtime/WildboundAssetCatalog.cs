using System;
using System.Collections.Generic;
using UnityEngine;

public enum WildboundAssetRole { Hero, Monster, Boss, Tree, Rock, Foliage, Environment, Vfx }

[Serializable]
public class SketchfabCredit
{
    public WildboundAssetRole role;
    public string modelName;
    public string creator;
    [TextArea] public string modelUrl;
    public string license = "CC BY";
    public string licenseUrl;
}

[CreateAssetMenu(fileName = "WildboundAssetCatalog", menuName = "Wildbound/Sketchfab Asset Catalog")]
public sealed class WildboundAssetCatalog : ScriptableObject
{
    [Header("Character prefabs downloaded from Sketchfab")]
    public GameObject heroPrefab;
    public GameObject monsterPrefab;
    public GameObject bossPrefab;

    [Header("Environment prefabs downloaded from Sketchfab")]
    public GameObject[] treePrefabs;
    public GameObject[] rockPrefabs;
    public GameObject[] foliagePrefabs;
    public GameObject[] extraEnvironmentPrefabs;

    [Header("Effects and audio files assigned locally")]
    public ParticleSystem swordHitVfx;
    public ParticleSystem abilityVfx;
    public AudioClip swordHitSfx;
    public AudioClip monsterRoarSfx;
    public AudioClip backgroundMusic;

    [Header("Sketchfab author and license credits")]
    public List<SketchfabCredit> credits = new List<SketchfabCredit>();
}

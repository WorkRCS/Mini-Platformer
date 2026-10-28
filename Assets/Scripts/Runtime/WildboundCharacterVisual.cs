using UnityEngine;

public sealed class WildboundCharacterVisual : MonoBehaviour
{
    [Header("Optional imported clips (drag clips from your Sketchfab FBX/GLB here)")]
    public AnimationClip idleClip;
    public AnimationClip walkClip;
    public AnimationClip runClip;
    public AnimationClip jumpClip;
    public AnimationClip attackClip;
    public AnimationClip hitClip;
    public AnimationClip deathClip;

    [Header("Optional Mecanim Animator Controller")]
    public Animator animator;

    private Animation legacyAnimation;
    private int lastState = -1;

    private void Awake()
    {
        if (animator == null) animator = GetComponentInChildren<Animator>();
        legacyAnimation = GetComponent<Animation>();
        if (legacyAnimation == null) legacyAnimation = GetComponentInChildren<Animation>();
        if (legacyAnimation == null && HasLegacyClips()) legacyAnimation = gameObject.AddComponent<Animation>();
        AddClip(idleClip, "Idle");
        AddClip(walkClip, "Walk");
        AddClip(runClip, "Run");
        AddClip(jumpClip, "Jump");
        AddClip(attackClip, "Attack");
        AddClip(hitClip, "Hit");
        AddClip(deathClip, "Death");
    }

    private bool HasLegacyClips()
    {
        return idleClip || walkClip || runClip || jumpClip || attackClip || hitClip || deathClip;
    }

    private void AddClip(AnimationClip clip, string name)
    {
        if (clip == null) return;
        if (legacyAnimation == null) legacyAnimation = gameObject.AddComponent<Animation>();
        if (legacyAnimation.GetClip(name) == null) legacyAnimation.AddClip(clip, name);
    }

    private bool AnimatorHas(string parameter)
    {
        if (animator == null || animator.runtimeAnimatorController == null) return false;
        foreach (AnimatorControllerParameter p in animator.parameters)
            if (p.name == parameter) return true;
        return false;
    }

    public void SetAnimation(int state, float speed)
    {
        if (animator != null && animator.runtimeAnimatorController != null)
        {
            if (AnimatorHas("Speed")) animator.SetFloat("Speed", speed);
            if (AnimatorHas("Grounded")) animator.SetBool("Grounded", state != 3);
            if (AnimatorHas("IsRunning")) animator.SetBool("IsRunning", state == 2);
            if (AnimatorHas("Attack") && state == 4 && lastState != 4) animator.SetTrigger("Attack");
            if (AnimatorHas("Jump") && state == 3 && lastState != 3) animator.SetTrigger("Jump");
            if (AnimatorHas("Hit") && state == 5 && lastState != 5) animator.SetTrigger("Hit");
            if (AnimatorHas("Dead") && state == 6 && lastState != 6) animator.SetTrigger("Dead");
        }

        if (legacyAnimation != null)
        {
            string clip = ClipName(state);
            if (legacyAnimation.GetClip(clip) != null && (lastState != state || !legacyAnimation.isPlaying))
            {
                legacyAnimation.CrossFade(clip, 0.12f);
                legacyAnimation[clip].wrapMode = state == 4 || state == 5 || state == 6 ? WrapMode.Once : WrapMode.Loop;
            }
        }
        lastState = state;
    }

    private string ClipName(int state)
    {
        switch (state)
        {
            case 1: return walkClip ? "Walk" : "Idle";
            case 2: return runClip ? "Run" : (walkClip ? "Walk" : "Idle");
            case 3: return jumpClip ? "Jump" : "Idle";
            case 4: return attackClip ? "Attack" : "Idle";
            case 5: return hitClip ? "Hit" : "Idle";
            case 6: return deathClip ? "Death" : "Idle";
            default: return "Idle";
        }
    }
}

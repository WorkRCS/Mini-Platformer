using UnityEngine;

[RequireComponent(typeof(Camera))]
public sealed class WildboundCameraRig : MonoBehaviour
{
    public Transform target;
    public float distance = 7.5f;
    public float pitch = 16f;
    public float sensitivity = 2.4f;
    public Vector3 targetOffset = new Vector3(0, 1.25f, 0);

    private float yaw = 180f;
    private Vector2 lastPointer;
    private bool pointerWasHeld;

    private void LateUpdate()
    {
        if (target == null)
        {
            WildboundGame game = FindObjectOfType<WildboundGame>();
            if (game != null) target = game.PlayerTransform;
            if (target == null) return;
        }

        if (Input.GetMouseButton(1))
        {
            Vector2 now = Input.mousePosition;
            if (pointerWasHeld)
            {
                Vector2 delta = now - lastPointer;
                yaw += delta.x * sensitivity * 0.12f;
                pitch = Mathf.Clamp(pitch - delta.y * sensitivity * 0.08f, -5f, 55f);
            }
            lastPointer = now;
            pointerWasHeld = true;
        }
        else pointerWasHeld = false;

        if (Application.isMobilePlatform && Input.touchCount > 0)
        {
            Touch touch = Input.GetTouch(0);
            if (touch.position.x > Screen.width * 0.38f && touch.phase == TouchPhase.Moved)
            {
                yaw += touch.deltaPosition.x * sensitivity * 0.08f;
                pitch = Mathf.Clamp(pitch - touch.deltaPosition.y * sensitivity * 0.05f, -5f, 55f);
            }
        }

        Vector3 focus = target.position + targetOffset;
        Quaternion orbit = Quaternion.Euler(pitch, yaw, 0);
        transform.position = focus + orbit * new Vector3(0, 0.15f, -distance);
        transform.rotation = Quaternion.LookRotation(focus - transform.position, Vector3.up);
    }
}

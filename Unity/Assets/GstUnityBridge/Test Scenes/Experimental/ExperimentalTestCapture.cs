using UnityEngine;

#if EXPERIMENTAL
public class ExperimentalTestCapture : MonoBehaviour
{

    // Use this for initialization
    void Start()
    {

    }

    // Update is called once per frame
    void LateUpdate()
    {
        RenderTexture rt = new RenderTexture(640, 480, 24);
        Camera cam = GetComponent<Camera>();
        cam.targetTexture = rt;
        RenderTexture.active = rt;
        cam.Render();
        Texture2D tex = new Texture2D(640, 480, TextureFormat.RGB24, false);
        tex.ReadPixels(new Rect(0, 0, 640, 480), 0, 0);
        GstUnityBridgeCapture capture = GameObject.Find("Recorder").GetComponent<GstUnityBridgeCapture>();
        capture.m_Source = tex;
        cam.targetTexture = null;
        RenderTexture.active = null;
    }
}
#endif

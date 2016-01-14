/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
 */

using UnityEngine;

[DisallowMultipleComponent]
public class GstUnityBridgeTexture : MonoBehaviour
{
    public bool m_FlipX = false;
    public bool m_FlipY = false;
    [Tooltip("URI to get the stream from")]
    public string m_URI = "";
    [Tooltip("SDP index of the video stream to use (-1 disables video)")]
    public int m_VideoIndex = 0;
    [Tooltip("SDP index of the audio stream to use (-1 disables audio)")]
    public int m_AudioIndex = 0;

    [SerializeField]
    [Tooltip("Leave always ON, unless you plan to activate it manually")]
    private bool m_InitializeOnStart = true;
    private bool m_HasBeenInitialized = false;

    [Header("Network synchronization")]
    [Tooltip("If unchecked, next two items are unused")]
    public bool m_UseNetworkSynchronization = false;
    [Tooltip("IP address or host name of the clock provider")]
    public string m_ClockAddress = "";
    [Tooltip("Port of the clock provider")]
    public int m_ClockPort = 0;

    private GstUnityBridgePipeline m_Pipeline;
    private Texture2D m_Texture = null;
    private int m_Width = 64;
    private int m_Height = 64;

    public void Initialize()
    {
        m_HasBeenInitialized = true;

        GStreamer.Ref();
        m_Pipeline = new GstUnityBridgePipeline();

        // Call resize which will create a texture and a webview for us if they do not exist yet at this point.
        Resize(m_Width, m_Height);

        if (GetComponent<GUITexture>())
        {
            GetComponent<GUITexture>().texture = m_Texture;
        }
        else if (GetComponent<Renderer>() && GetComponent<Renderer>().material)
        {
            GetComponent<Renderer>().material.mainTexture = m_Texture;
            GetComponent<Renderer>().material.mainTextureScale = new Vector2(Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.x) * (m_FlipX ? -1.0f : 1.0f),
                                                                             Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.y) * (m_FlipY ? -1.0f : 1.0f));
        }
        else
        {
            Debug.LogWarning("There is no Renderer or guiTexture attached to this GameObject! GstTexture will render to a texture but it will not be visible.");
        }

    }

    void Start()
    {
        if (m_InitializeOnStart && !m_HasBeenInitialized)
        {
            Initialize();
            Setup(m_URI, m_VideoIndex, m_AudioIndex);
            Play();
        }
    }

    public void Resize(int _Width, int _Height)
    {
        m_Width = _Width;
        m_Height = _Height;

        if (m_Texture == null)
        {
            m_Texture = new Texture2D(m_Width, m_Height, TextureFormat.RGB24, false);
        }
        else
        {
            m_Texture.Resize(m_Width, m_Height, TextureFormat.RGB24, false);
            m_Texture.Apply(false, false);
        }
        m_Texture.filterMode = FilterMode.Bilinear;
    }

    public void Setup(string _URI, int _VideoIndex, int _AudioIndex)
    {
        m_URI = _URI;
        m_VideoIndex = _VideoIndex;
        m_AudioIndex = _AudioIndex;
        if (m_Pipeline.IsLoaded || m_Pipeline.IsPlaying)
            m_Pipeline.Close();
        m_Pipeline.Setup(m_URI, m_VideoIndex, m_AudioIndex,
            m_UseNetworkSynchronization ? m_ClockAddress : null, m_ClockPort);
    }

    public void Destroy()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.Destroy();
            m_Pipeline = null;
            GStreamer.Unref();
        }
    }

    public void Play()
    {
        m_Pipeline.Play();
    }

    public void Pause()
    {
        m_Pipeline.Pause();
    }

    public void Stop()
    {
        m_Pipeline.Stop();
    }

    public void Close()
    {
        m_Pipeline.Close();
    }

    void OnDestroy()
    {
        Destroy();
    }

    void OnApplicationQuit()
    {
        Destroy();
    }

    void OnGUI()
    {
        // This function should do input injection (if enabled), and drawing.
        if (m_Pipeline == null)
            return;

        Event e = Event.current;

        switch (e.type)
        {
            case EventType.Repaint:
                {
                    Vector2 sz;
                    if (m_Pipeline.GrabFrame(out sz))
                    {
                        Resize((int)sz.x, (int)sz.y);
                        if (m_Texture == null)
                            Debug.LogError("The GstTexture does not have a texture assigned and will not paint.");
                        else
                            m_Pipeline.BlitTexture(m_Texture.GetNativeTexturePtr(), m_Texture.width, m_Texture.height);
                    }
                    break;
                }
        }
    }
}

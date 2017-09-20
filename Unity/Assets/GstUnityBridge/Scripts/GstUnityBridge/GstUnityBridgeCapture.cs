using System.Runtime.InteropServices;
using UnityEngine;

#if EXPERIMENTAL

class GstUnityBridgeCapture : MonoBehaviour
{
    public Texture2D m_Source = null;
    public string m_Filename = null;

    private GstUnityBridgePipeline m_Pipeline;
    private GCHandle m_instanceHandle;

    private bool m_EOS = false;
    private int m_width = 0, m_height = 0;

    void Awake()
    {
        GStreamer.AddPluginsToPath();
    }

    void Update()
    {
        if (m_Source == null || m_Filename == null) return;

        if (m_width != m_Source.width || m_height != m_Source.height)
        {
            m_width = m_Source.width;
            m_height = m_Source.height;
            m_Pipeline.SetupEncoding(m_Filename, m_Source.width, m_Source.height);
            m_Pipeline.Play();
        }

        var pixels = m_Source.GetPixels32();
        var handle = GCHandle.Alloc(pixels, GCHandleType.Pinned);
        m_Pipeline.ConsumeImage(handle.AddrOfPinnedObject(), pixels.Length * 4);
        handle.Free();
    }


    private static void OnFinish(System.IntPtr p)
    {
        GstUnityBridgeCapture self = ((GCHandle)p).Target as GstUnityBridgeCapture;
        self.m_EOS = true;
    }

    void Initialize()
    {
        GStreamer.GUBUnityDebugLogPFN log_handler = null;
        if (Application.isEditor)
        {
            log_handler = (int level, string message) => Debug.logger.Log((LogType)level, "GUB", message);
        }

        GStreamer.Ref("2", log_handler);
        m_instanceHandle = GCHandle.Alloc(this);
        m_Pipeline = new GstUnityBridgePipeline(name + GetInstanceID(), OnFinish, null, null, (System.IntPtr)m_instanceHandle);
    }

    void Start()
    {
        Initialize();
        if (string.IsNullOrEmpty(m_Filename))
        {
            Debug.LogError("Please provide a filename");
            return;
        }
        if (m_Source != null)
        {
            m_Pipeline.SetupEncoding(m_Filename, m_Source.width, m_Source.height);
            m_Pipeline.Play();
        }
    }

    void OnDestroy()
    {
        Destroy();
    }

    void OnApplicationQuit()
    {
        Destroy();
    }

    private void Destroy()
    {
        if (m_Pipeline != null)
        {
            // Send EOS down the pipeline
            m_Pipeline.StopEncoding();
            // Wait for EOS to reach the end
            while (!m_EOS)
            {
                System.Threading.Thread.Sleep(1000);
            }
            m_Pipeline.Destroy();
            m_Pipeline = null;
            GStreamer.Unref();
            m_instanceHandle.Free();
        }
    }
}

#endif
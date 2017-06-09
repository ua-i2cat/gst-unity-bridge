using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GstUnityBridgeStarter : MonoBehaviour
{
    public bool GStreamerDebugActivated = true;
    private readonly string GStreamerDebugString = "2,dashdemux:5";

    /// <summary>
	/// Initializes the GStreamer on Awake
	/// </summary>
    void Awake()
    {
        LoadGStreamer();
    }

    /// <summary>
    /// When Application is closed calls UnloadGStreamer method
    /// </summary>
    void OnApplicationQuit()
    {
        UnLoadGStreamer();
    }

    /// <summary>
    /// Loads GStreamer and makes the ref for the GStreamer's Log_handler.
    /// </summary>
    void LoadGStreamer()
    {
        GStreamer.GUBUnityDebugLogPFN log_handler = null;
        if (Application.isEditor && GStreamerDebugActivated)
        {
            log_handler = (int level, string message) => Debug.logger.Log((LogType)level, "GUB", message);
        }

        GStreamer.Ref(GStreamerDebugString.Length == 0 ? null : GStreamerDebugString, log_handler);

        GStreamer.AddPluginsToPath();
    }


    /// <summary>
    /// Unload the GStreamer's Log_handler
    /// </summary>
    void UnLoadGStreamer()
    {
        GStreamer.Unref();
    }
}

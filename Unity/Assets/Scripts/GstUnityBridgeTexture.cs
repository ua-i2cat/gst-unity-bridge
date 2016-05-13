/*
*  GStreamer - Unity3D bridge (GUB).
*  Copyright (C) 2016  Fundacio i2CAT, Internet i Innovacio digital a Catalunya
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  Authors:  Xavi Artigas <xavi.artigas@i2cat.net>
*/

using UnityEngine;
using UnityEngine.Events;
using System;
using System.IO;
using System.Runtime.InteropServices;

[Serializable]
public class GstUnityBridgeCroppingParams
{
    [Tooltip("Amount to crop from the left margin")]
    [Range(0, 1)]
    public float m_Left = 0.0F;
    [Tooltip("Amount to crop from the top margin")]
    [Range(0, 1)]
    public float m_Top = 0.0F;
    [Tooltip("Amount to crop from the right margin")]
    [Range(0, 1)]
    public float m_Right = 0.0F;
    [Tooltip("Amount to crop from the bottom margin")]
    [Range(0, 1)]
    public float m_Bottom = 0.0F;
};

[Serializable]
public class GstUnityBridgeSynchronizationParams
{
    [Tooltip("If unchecked, next two items are unused")]
    public bool m_Enabled = false;
    [Tooltip("IP address or host name of the GStreamer network clock provider")]
    public string m_MasterClockAddress = "";
    [Tooltip("Port of the GStreamer network clock provider")]
    public int m_MasterClockPort = 0;
}

[Serializable]
public class GstUnityBridgeDebugParams
{
    [Tooltip("Enable to write debug output to the Unity Editor Console, " +
        "LogCat on Android or gub.txt on a Standalone player")]
    public bool m_Enabled = false;
    [Tooltip("Comma-separated list of categories and log levels as used " +
        "with the GST_DEBUG environment variable. Setting to '2' is normally enough. " +
        "Leave empty to disable GStreamer debug output.")]
    public string m_GStreamerDebugString = "2";
}

[Serializable]
public class StringEvent : UnityEvent<string> { }

[Serializable]
public class GstUnityBridgeEventParams
{
    [Tooltip("Called when media reaches the end")]
    public UnityEvent m_OnFinish;
    [Tooltip("Called when GStreamer reports an error")]
    public StringEvent m_OnError;
}

public class GstUnityBridgeTexture : MonoBehaviour
{
#if !EXPERIMENTAL
    [HideInInspector]
#endif
    [Tooltip("The output will be written to a texture called '_AlphaTex' instead of the main texture " +
        "(Requires the ExternalAlpha shader)")]
    public bool m_IsAlpha = false;
    public bool m_FlipX = false;
    public bool m_FlipY = false;
    [Tooltip("URI to get the stream from")]
    public string m_URI = "";
    [Tooltip("Zero-based index of the video stream to use (-1 disables video)")]
    public int m_VideoIndex = 0;
    [Tooltip("Zero-based index of the audio stream to use (-1 disables audio)")]
    public int m_AudioIndex = 0;

    [SerializeField]
    [Tooltip("Leave always ON, unless you plan to activate it manually")]
    public bool m_InitializeOnStart = true;
    private bool m_HasBeenInitialized = false;

    public GstUnityBridgeEventParams m_Events;
    public GstUnityBridgeCroppingParams m_VideoCropping;
    public GstUnityBridgeSynchronizationParams m_NetworkSynchronization;
    public GstUnityBridgeDebugParams m_DebugOutput;

    private GstUnityBridgePipeline m_Pipeline;
    private Texture2D m_Texture = null;
    private int m_Width = 64;
    private int m_Height = 64;
    private EventProcessor m_EventProcessor;
    private GCHandle m_instanceHandle;

    void Awake()
    {
        // Setup the PATH environment variable so it can find the GstUnityBridge dll.
        var currentPath = Environment.GetEnvironmentVariable("PATH",
            EnvironmentVariableTarget.Process);
        var dllPath = "";

#if UNITY_EDITOR

#if UNITY_EDITOR_32
        dllPath = Application.dataPath + "/Plugins/x86";
#elif UNITY_EDITOR_64
        dllPath = Application.dataPath + "/Plugins/x86_64";
#endif

        if (currentPath != null && currentPath.Contains(dllPath) == false)
            Environment.SetEnvironmentVariable("PATH",
                dllPath + Path.PathSeparator +
                dllPath + "/GStreamer/bin" + Path.PathSeparator +
                currentPath,
                EnvironmentVariableTarget.Process);
#else
        dllPath = Application.dataPath + "/Plugins";
        if (currentPath != null && currentPath.Contains(dllPath) == false)
            Environment.SetEnvironmentVariable("PATH",
                dllPath + Path.PathSeparator +
                currentPath,
                EnvironmentVariableTarget.Process);
        Environment.SetEnvironmentVariable("GST_PLUGIN_PATH", dllPath, EnvironmentVariableTarget.Process);
#endif
    }

    private static void OnFinish(IntPtr p)
    {
        GstUnityBridgeTexture self = ((GCHandle)p).Target as GstUnityBridgeTexture;

        self.m_EventProcessor.QueueEvent(() => {
            if (self.m_Events.m_OnFinish != null)
            {
                self.m_Events.m_OnFinish.Invoke();
            }
        });
    }

    private static void OnError(IntPtr p, string message)
    {
        GstUnityBridgeTexture self = ((GCHandle)p).Target as GstUnityBridgeTexture;

        self.m_EventProcessor.QueueEvent(() => {
            if (self.m_Events.m_OnError != null)
            {
                self.m_Events.m_OnError.Invoke(message);
            }
        });
    }

    public void Initialize()
    {
        m_HasBeenInitialized = true;

        m_EventProcessor = GetComponent<EventProcessor>();
        if (m_EventProcessor == null)
        {
            m_EventProcessor = gameObject.AddComponent<EventProcessor>();
        }

        if (Application.isEditor && m_DebugOutput.m_Enabled)
        {
            GStreamer.gub_log_set_unity_handler(
                (int level, string message) => Debug.logger.Log((LogType)level, "GUB", message));
        } else
        {
            GStreamer.gub_log_set_unity_handler(null);

        }

        GStreamer.Ref(m_DebugOutput.m_GStreamerDebugString.Length == 0 ?
            null : m_DebugOutput.m_GStreamerDebugString);

        m_instanceHandle = GCHandle.Alloc(this);
        m_Pipeline = new GstUnityBridgePipeline(name + GetInstanceID(), OnFinish, OnError, (IntPtr)m_instanceHandle);

        // Call resize which will create a texture and a webview for us if they do not exist yet at this point.
        Resize(m_Width, m_Height);

        if (GetComponent<GUITexture>())
        {
            GetComponent<GUITexture>().texture = m_Texture;
        }
        else if (GetComponent<Renderer>() && GetComponent<Renderer>().material)
        {
            if (!m_IsAlpha)
            {
                GetComponent<Renderer>().material.mainTexture = m_Texture;
                GetComponent<Renderer>().material.mainTextureScale = new Vector2(Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.x) * (m_FlipX ? -1.0f : 1.0f),
                                                                                 Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.y) * (m_FlipY ? -1.0f : 1.0f));
            } else
            {
                GetComponent<Renderer>().material.SetTexture("_AlphaTex", m_Texture);
                GetComponent<Renderer>().material.SetTextureScale("_AlphaTex", new Vector2(Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.x) * (m_FlipX ? -1.0f : 1.0f),
                                                                                 Mathf.Abs(GetComponent<Renderer>().material.mainTextureScale.y) * (m_FlipY ? -1.0f : 1.0f)));
            }
        }
        else
        {
            Debug.LogWarning(string.Format("[{0}] There is no Renderer or guiTexture attached to this GameObject! GstTexture will render to a texture but it will not be visible.", name));
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
            m_NetworkSynchronization.m_Enabled ? m_NetworkSynchronization.m_MasterClockAddress : null,
            m_NetworkSynchronization.m_MasterClockPort,
            m_VideoCropping.m_Left, m_VideoCropping.m_Top, m_VideoCropping.m_Right, m_VideoCropping.m_Bottom);
    }

    public void Destroy()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.Destroy();
            m_Pipeline = null;
            GStreamer.Unref();
        }
        m_instanceHandle.Free();
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

    public double Duration
    {
        get { return m_Pipeline != null ? m_Pipeline.Duration : 0F; }
    }

    public double Position
    {
        get { return m_Pipeline != null ? m_Pipeline.Position : 0F; }
        set { if (m_Pipeline !=null) m_Pipeline.Position = value; }
    }

    void Update()
    {
        if (m_Pipeline == null)
            return;

        Vector2 sz;
        if (m_Pipeline.GrabFrame(out sz))
        {
            Resize((int)sz.x, (int)sz.y);
            if (m_Texture == null)
                Debug.LogError(string.Format("[{0}] The GstTexture does not have a texture assigned and will not paint.", name));
            else
                m_Pipeline.BlitTexture(m_Texture.GetNativeTexturePtr(), m_Texture.width, m_Texture.height);
        }
    }
}

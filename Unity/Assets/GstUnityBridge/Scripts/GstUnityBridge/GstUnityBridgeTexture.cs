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
    [Tooltip("If unchecked, the rest of the items are unused")]
    public bool m_Enabled = false;
    [Tooltip("IP address or host name of the GStreamer network clock provider")]
    public string m_MasterClockAddress = "";
    [Tooltip("Port of the GStreamer network clock provider")]
    public int m_MasterClockPort = 0;
    [Tooltip("Activate the Dvb Wallclock system, instead of GStreamers default")]
    public bool m_isDvbWC = false;
#if !EXPERIMENTAL
    [HideInInspector]
#endif
    [Tooltip("Basetime allows connecting to an already playing pipeline, " +
        "by using the same basetime on all players. Set to 0 to start " +
        "playing the media from the beginning (normal use).")]
    public ulong m_BaseTime = 0;
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

// To understand the QoS (Quality of Service) data, read this:
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/pwg/html/chapter-advanced-qos.html
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstMessage.html#gst-message-parse-qos
[Serializable]
public class QosData
{
    // Nanoseconds. How late a buffer was. Negative is early, positive is late.
    // Early buffers are not reported, obviously.
    public long current_jitter;
    // Nanoseconds. Running time when the late buffer was reported.
    public ulong current_running_time;
    // Nanoseconds. Stream time when the late buffer was reported.
    public ulong current_stream_time;
    // Nanoseconds. Timestamp of the late buffer.
    public ulong current_timestamp;
    // Long term prediction of the ideal rate relative to normal rate to get optimal quality.
    public double proportion;
    // Total number of correctly processed frames.
    public ulong processed;
    // Total number of dropped frames, because they were late.
    public ulong dropped;
}

[Serializable]
public class QosEvent : UnityEvent<QosData> { }

[Serializable]
public class GstUnityBridgeEventParams
{
    [Header("Called when the first frame is available")]
    public UnityEvent m_OnStart;
    [Header("Called when media reaches the end")]
    public UnityEvent m_OnFinish;
    [Header("Called when GStreamer reports an error")]
    public StringEvent m_OnError;
    [Header("Called when a Quality Of Service event occurs")]
    public QosEvent m_OnQOS;
}

public class GstUnityBridgeTexture : MonoBehaviour
{
#if !EXPERIMENTAL
    [HideInInspector]
#endif
    [Tooltip("The output will be written to a texture called '_AlphaTex' instead of the main texture " +
        "(Requires the ExternalAlpha shader)")]
    public bool m_IsAlpha = false;
    [Tooltip("Flip texture horizontally")]
    public bool m_FlipX = false;
    [Tooltip("Flip texture vertically")]
    public bool m_FlipY = false;
    [Tooltip("Play media from the beginning when it reaches the end")]
    public bool m_Loop = false;
    [Tooltip("URI to get the stream from")]
    public string m_URI = "";
    [Tooltip("Zero-based index of the video stream to use (-1 disables video)")]
    public int m_VideoIndex = 0;
    [Tooltip("Zero-based index of the audio stream to use (-1 disables audio)")]
    public int m_AudioIndex = 0;

    [Tooltip("Optional material whose texture will be replaced. If None, the first material in the Renderer of this GameObject will be used.")]
    public Material m_TargetMaterial;

    [Tooltip("From 0 (mute) to 1 (max volume)")]
    [Range(0,1)]
    public double m_AudioVolume = 1.0F;

    [Tooltip("When using Adaptive Streaming (DASH or HLS) this allows setting a limit on the bitrate from 0 (minimum quality) to 1 (maximum quality)")]
    [Range(0, 1)]
    public float m_AdaptiveBitrateLimit = 1.0F;

    [SerializeField]
    [Tooltip("Leave always ON, unless you plan to activate it manually")]
    public bool m_InitializeOnStart = true;
    private bool m_HasBeenInitialized = false;

    public GstUnityBridgeEventParams m_Events = new GstUnityBridgeEventParams();
    public GstUnityBridgeCroppingParams m_VideoCropping = new GstUnityBridgeCroppingParams();
    public GstUnityBridgeSynchronizationParams m_NetworkSynchronization = new GstUnityBridgeSynchronizationParams();
    public GstUnityBridgeDebugParams m_DebugOutput = new GstUnityBridgeDebugParams();

    private GstUnityBridgePipeline m_Pipeline = null;
    private Texture2D m_Texture = null;
    private int m_Width = 64;
    private int m_Height = 64;
    private EventProcessor m_EventProcessor = null;
    private GCHandle m_instanceHandle;
    private bool m_FirstFrame = true;
	

    private static void OnFinish(IntPtr p)
    {
        GstUnityBridgeTexture self = ((GCHandle)p).Target as GstUnityBridgeTexture;

        self.m_EventProcessor.QueueEvent(() =>
        {
            if (self != null)
            {
                if (self.m_Events.m_OnFinish != null)
                {
                    self.m_Events.m_OnFinish.Invoke();
                }
                if (self.m_Loop)
                {
                    self.Position = 0;
                }
            }
        });
    }

    private static void OnError(IntPtr p, string message)
    {
        GstUnityBridgeTexture self = ((GCHandle)p).Target as GstUnityBridgeTexture;

        if (self != null)
        {
            if (self.m_Events.m_OnError != null)
            {
                self.m_EventProcessor.QueueEvent(() =>
                {
                    self.m_Events.m_OnError.Invoke(message);
                });
            }
        }
    }

    private static void OnQos(IntPtr p,
        long current_jitter, ulong current_running_time, ulong current_stream_time, ulong current_timestamp,
        double proportion, ulong processed, ulong dropped)
    {
        GstUnityBridgeTexture self = ((GCHandle)p).Target as GstUnityBridgeTexture;

        if (self != null)
        {
            if (self.m_Events.m_OnQOS != null)
            {
                self.m_EventProcessor.QueueEvent(() =>
                {
                    QosData data = new QosData()
                    {
                        current_jitter = current_jitter,
                        current_running_time = current_running_time,
                        current_stream_time = current_stream_time,
                        current_timestamp = current_timestamp,
                        proportion = proportion,
                        processed = processed,
                        dropped = dropped
                    };
                    self.m_Events.m_OnQOS.Invoke(data);
                });
            }
        }
    }

    public void Initialize()
    {
        if (!m_HasBeenInitialized)
        {
            m_HasBeenInitialized = true;

            m_EventProcessor = GetComponent<EventProcessor>();
            if (m_EventProcessor == null)
            {
                m_EventProcessor = gameObject.AddComponent<EventProcessor>();
            }

            m_instanceHandle = GCHandle.Alloc(this);

            m_Pipeline = new GstUnityBridgePipeline(name + GetInstanceID(), OnFinish, OnError, OnQos, (IntPtr)m_instanceHandle);

            Resize(m_Width, m_Height);

            Material mat = m_TargetMaterial;
            if (mat == null && GetComponent<Renderer>())
            {
                // If no material is given, use the first one in the Renderer component
                mat = GetComponent<Renderer>().material;
            }

            if (mat != null)
            {
                string tex_name = m_IsAlpha ? "_AlphaTex" : "_MainTex";
                mat.SetTexture(tex_name, m_Texture);
                mat.SetTextureScale(tex_name, new Vector2(Mathf.Abs(mat.mainTextureScale.x) * (m_FlipX ? -1F : 1F),
                                                          Mathf.Abs(mat.mainTextureScale.y) * (m_FlipY ? -1F : 1F)));
            }
            else
            if (GetComponent<GUITexture>())
            {
                GetComponent<GUITexture>().texture = m_Texture;
            }
            else
            {
                Debug.LogWarning(string.Format("[{0}] There is no Renderer or guiTexture attached to this GameObject, and TargetMaterial is not set.", name + GetInstanceID()));
            }
        }
    }

    void Start()
    {
        if (m_InitializeOnStart && !m_HasBeenInitialized)
        {
            InitializeSetupPlay();
        }
    }

   public void InitializeSetupPlay()
    {

        Initialize();
        Setup(m_URI, m_VideoIndex, m_AudioIndex);
        Play();
    }
	
	public bool HasBeenInitialized()
    {
        return m_HasBeenInitialized;
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
        m_Pipeline.SetupDecoding(m_URI, m_VideoIndex, m_AudioIndex,
            m_NetworkSynchronization.m_Enabled ? m_NetworkSynchronization.m_MasterClockAddress : null,
            m_NetworkSynchronization.m_MasterClockPort,
            m_NetworkSynchronization.m_BaseTime,
            m_VideoCropping.m_Left, m_VideoCropping.m_Top, m_VideoCropping.m_Right, m_VideoCropping.m_Bottom,
            m_NetworkSynchronization.m_isDvbWC);
    }

    public void Destroy()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.Destroy();
            m_Pipeline = null;
        }
        m_instanceHandle.Free();
        Debug.Log("[GUB] Destroy");
    }

    public void Play()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.SetVolume(m_AudioVolume);
            m_Pipeline.Play();
            m_FirstFrame = true;
        }
    }

    public void Pause()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.Pause();
        }
    }

    public void Stop()
    {
        if(m_Pipeline != null)
        {
            m_Pipeline.Stop();
        }
    }

    public void Close()
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.Close();
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

    public double Duration
    {
        get { return m_Pipeline != null ? m_Pipeline.Duration : 0F; }
    }

    public double Position
    {
        get { return m_Pipeline != null ? m_Pipeline.Position : 0F; }
        set { if (m_Pipeline != null) m_Pipeline.Position = value; }
    }

    public ulong Basetime
    {
        get { return m_Pipeline != null ? m_NetworkSynchronization.m_BaseTime : 0; }
        set { if (m_Pipeline != null) m_Pipeline.Basetime = m_NetworkSynchronization.m_BaseTime = value; }
    }

    public bool IsPlaying
    {
        get { return m_Pipeline != null ? m_Pipeline.IsPlaying : false; }
    }

    public void SetVolume(double volume)
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.SetVolume(volume);
        }
        m_AudioVolume = volume;
    }

    public void SetAdaptiveBitrateLimit(float bitrate_limit)
    {
        if (m_Pipeline != null)
        {
            m_Pipeline.SetAdaptiveBitrateLimit(bitrate_limit);
        }
        m_AdaptiveBitrateLimit = bitrate_limit;
    }

    void Update()
    {
        if (m_Pipeline == null)
            return;

        Vector2 sz = Vector2.zero;
        if (m_Pipeline.GrabFrame(ref sz))
        {
            Resize((int)sz.x, (int)sz.y);
            if (m_Texture == null)
            {
                Debug.LogWarning(string.Format("[{0}] The GUBTexture does not have a texture assigned and will not paint.", name + GetInstanceID()));
            }
            else
            {
                m_Pipeline.BlitTexture(m_Texture.GetNativeTexturePtr(), m_Texture.width, m_Texture.height);
            }

            if (m_FirstFrame)
            {
                if (m_AdaptiveBitrateLimit != 1.0F)
                {
                    SetAdaptiveBitrateLimit(m_AdaptiveBitrateLimit);
                }

                if (m_Events.m_OnStart != null)
                {
                    m_Events.m_OnStart.Invoke();
                }

                m_FirstFrame = false;
            }
        }
    }
}

/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
 */

using UnityEngine;
using System.Runtime.InteropServices;	// For DllImport.
using System;

public class GStreamer
{
    internal const string DllName = "GstUnityBridge";

    [DllImport("gstreamer_android", CallingConvention = CallingConvention.Cdecl)]
    extern static private UIntPtr gst_android_get_application_class_loader();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I4)]
    extern static private bool gub_ref();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_unref();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I4)]
    extern static private bool gub_is_active();

    public static bool IsActive
    {
        get
        {
            return gub_is_active();
        }
    }

    public static void Ref()
    {
#if UNITY_ANDROID
        // Force loading of gstreamer_android.so before GstUnityBridge.so
        gst_android_get_application_class_loader();
        AndroidJNIHelper.debug = true;
        AndroidJavaClass unityPlayer = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
        AndroidJavaObject activity = unityPlayer.GetStatic<AndroidJavaObject>("currentActivity");
        AndroidJavaClass gstAndroid = new AndroidJavaClass("org.freedesktop.gstreamer.GStreamer");
        gstAndroid.CallStatic("init", activity);
#endif
        gub_ref();
    }

    public static void Unref()
    {
        gub_unref();
    }
}

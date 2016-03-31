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

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
        gub_ref();
    }

    public static void Unref()
    {
        gub_unref();
    }
}

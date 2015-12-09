/*
 * GStreamer - Unity3D bridge.
 * Based on https://github.com/mrayy/mrayGStreamerUnity
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

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_set_debug_function(IntPtr str);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void MyDelegate(string str);

    static void CallBackFunction(string str)
    {
        Debug.Log("GstUnityBridge: " + str);
    }

    public static bool IsActive
    {
        get
        {
            return gub_is_active();
        }
    }

    public static void Ref()
    {
        if (!IsActive)
        {
            MyDelegate callback = new MyDelegate(CallBackFunction);
            IntPtr intptr_del = Marshal.GetFunctionPointerForDelegate(callback);
            gub_set_debug_function(intptr_del);
        }
        gub_ref();
    }

    public static void Unref()
    {
        if (IsActive)
            gub_unref();
    }
}

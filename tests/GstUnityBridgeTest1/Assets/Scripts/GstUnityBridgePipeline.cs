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

public class GstUnityBridgePipeline
{
    internal const string DllName = "GstUnityBridge";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_destroy(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_play(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_pause(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_stop(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private bool gub_pipeline_is_loaded(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private bool gub_pipeline_is_playing(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_close(System.IntPtr p);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private System.IntPtr gub_pipeline_create();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_setup(System.IntPtr p,
        [MarshalAs(UnmanagedType.LPStr)]string uri,
        int video_index,
        int audio_index,
        [MarshalAs(UnmanagedType.LPStr)]string net_clock_address,
        int net_clock_port,
        float crop_left, float crop_top, float crop_right, float crop_bottom);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private int gub_pipeline_grab_frame(System.IntPtr p, ref int w, ref int h);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    extern static private void gub_pipeline_blit_image(System.IntPtr p, System.IntPtr _TextureNativePtr, int _UnityTextureWidth, int _UnityTextureHeight);

    protected System.IntPtr m_Instance;

    public bool IsLoaded
    {
        get
        {
            return gub_pipeline_is_loaded(m_Instance);
        }
    }

    public bool IsPlaying
    {
        get
        {
            return gub_pipeline_is_playing(m_Instance);
        }
    }

    public System.IntPtr NativePtr
    {
        get
        {
            return m_Instance;
        }
    }

    public void Destroy()
    {
        gub_pipeline_destroy(m_Instance);
    }

    public void Play()
    {
        gub_pipeline_play(m_Instance);
    }

    public void Pause()
    {
        gub_pipeline_pause(m_Instance);
    }

    public void Stop()
    {
        gub_pipeline_stop(m_Instance);
    }

    public void Close()
    {
        gub_pipeline_close(m_Instance);
    }

    public GstUnityBridgePipeline()
    {
        m_Instance = gub_pipeline_create();
    }

    public void Setup(string uri, int video_index, int audio_index, string net_clock_address, int net_clock_port, float crop_left, float crop_top, float crop_right, float crop_bottom)
    {
        gub_pipeline_setup(m_Instance, uri, video_index, audio_index, net_clock_address, net_clock_port, crop_left, crop_top, crop_right, crop_bottom);
    }

    public bool GrabFrame(out Vector2 frameSize)
    {
        int w = 0, h = 0;
        if (gub_pipeline_grab_frame(m_Instance, ref w, ref h) == 1)
        {
            frameSize.x = w;
            frameSize.y = h;
            return true;
        }
        frameSize.x = frameSize.y = 0;
        return false;
    }

    public void BlitTexture(System.IntPtr _NativeTexturePtr, int _TextureWidth, int _TextureHeight)
    {
        if (_NativeTexturePtr == System.IntPtr.Zero) return;

        gub_pipeline_blit_image(m_Instance, _NativeTexturePtr, _TextureWidth, _TextureHeight);  // We pass Unity's width and height values of the texture
    }
}

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
*            Ibai Jurado <ibai.jurado@i2cat.net>
*/

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GstUnityBridgeStarter : MonoBehaviour
{
	[Tooltip("Enable the debug logs for GStreamer")]
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

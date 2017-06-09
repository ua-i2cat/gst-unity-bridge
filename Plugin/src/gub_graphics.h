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

#include "gub.h"

/* Copied from IUnityGraphics.h and added some bits */
typedef enum UnityGfxRenderer {
    kUnityGfxRendererOpenGL = 0, // Desktop OpenGL
    kUnityGfxRendererD3D9 = 1, // Direct3D 9
    kUnityGfxRendererD3D11 = 2, // Direct3D 11
    kUnityGfxRendererGCM = 3, // PlayStation 3
    kUnityGfxRendererNull = 4, // "null" device (used in batch mode)
    kUnityGfxRendererXenon = 6, // Xbox 360
    kUnityGfxRendererOpenGLES20 = 8, // OpenGL ES 2.0
    kUnityGfxRendererOpenGLES30 = 11, // OpenGL ES 3.0
    kUnityGfxRendererGXM = 12, // PlayStation Vita
    kUnityGfxRendererPS4 = 13, // PlayStation 4
    kUnityGfxRendererXboxOne = 14, // Xbox One
    kUnityGfxRendererMetal = 16, // iOS Metal
    kUnityGfxRendererGLCore = 17, // OpenGL Core
    kUnityGfxRendererD3D12 = 18, // Direct3D 12
} UnityGfxRenderer;

typedef enum UnityGfxDeviceEventType {
    kUnityGfxDeviceEventInitialize = 0,
    kUnityGfxDeviceEventShutdown = 1,
    kUnityGfxDeviceEventBeforeReset = 2,
    kUnityGfxDeviceEventAfterReset = 3,
} UnityGfxDeviceEventType;

// --------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Unity integration ------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType);

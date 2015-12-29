/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
 */

#include "gub.h"
#include <stdio.h>

#if defined(_WIN32)
#define SUPPORT_OPENGL 1
#define SUPPORT_D3D9 1
#define SUPPORT_D3D11 1
#elif defined(__ANDROID__)
#define SUPPORT_EGL 1
#else
#define SUPPORT_OPENGL 1
#endif

/* Copied from IUnityGraphics.h */
typedef enum UnityGfxRenderer
{
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
	kUnityGfxRendererD3D12 = 18, // Direct3D 12
} UnityGfxRenderer;

typedef enum UnityGfxDeviceEventType
{
	kUnityGfxDeviceEventInitialize = 0,
	kUnityGfxDeviceEventShutdown = 1,
	kUnityGfxDeviceEventBeforeReset = 2,
	kUnityGfxDeviceEventAfterReset = 3,
} UnityGfxDeviceEventType;

static UnityGfxRenderer gub_renderer = -1;
static void *gub_device = NULL;

// Add alpha channel, OMFG
static void gub_add_alpha_channel(const char *src, char *dst, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		dst[i * 4 + 0] = src[i * 3 + 0];
		dst[i * 4 + 1] = src[i * 3 + 1];
		dst[i * 4 + 2] = src[i * 3 + 2];
		dst[i * 4 + 3] = 255;
	}
}

#if SUPPORT_D3D9
// ----------------- D3D9 SUPPORT -----------------
#include <d3d9.h>

static void gub_copy_texture_D3D9(const char *data, int w, int h, void *native_texture_ptr)
{
	IDirect3DDevice9* device = (IDirect3DDevice9*)gub_device;
	if (native_texture_ptr)
	{
		IDirect3DTexture9* d3dtex = (IDirect3DTexture9*)native_texture_ptr;
		D3DSURFACE_DESC desc;
		d3dtex->lpVtbl->GetLevelDesc(d3dtex, 0, &desc);
		D3DLOCKED_RECT lr;
		d3dtex->lpVtbl->LockRect(d3dtex, 0, &lr, NULL, 0);
		gub_add_alpha_channel(data, (char*)lr.pBits, w * h);
		d3dtex->lpVtbl->UnlockRect(d3dtex, 0);
	}
}

#endif

#if SUPPORT_D3D11
// ----------------- D3D11 SUPPORT -----------------
#include <d3d11.h>

static void gub_copy_texture_D3D11(const char *data, int w, int h, void *native_texture_ptr)
{
	ID3D11Device* device = (ID3D11Device*)gub_device;
	ID3D11DeviceContext* ctx = NULL;
	device->lpVtbl->GetImmediateContext(device, &ctx);
	// update native texture from code
	if (native_texture_ptr) {
		char *data2 = malloc(w * h * 4);
		gub_add_alpha_channel(data, data2, w*h);
		ctx->lpVtbl->UpdateSubresource(ctx, (ID3D11Resource *)native_texture_ptr, 0, NULL, data2, w * 4, 0);
		free(data2);
	}
	ctx->lpVtbl->Release(ctx);
}

#endif

#if SUPPORT_OPENGL
// ----------------- DESKTOP OPENGL SUPPORT -----------------
#include <GL/gl.h>

static void gub_copy_texture_OpenGL(const char *data, int w, int h, void *native_texture_ptr)
{
	if (native_texture_ptr)
	{
		GLuint gltex = (GLuint)(size_t)(native_texture_ptr);
		glBindTexture(GL_TEXTURE_2D, gltex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
}

#endif

#if SUPPORT_EGL
// ----------------- OPENGL-ES SUPPORT -----------------
#include <GLES/gl.h>

static void gub_copy_texture_EGL(const char *data, int w, int h, void *native_texture_ptr)
{
	if (native_texture_ptr)
	{
		GLuint gltex = (GLuint)(size_t)(native_texture_ptr);
		glBindTexture(GL_TEXTURE_2D, gltex);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
}

#endif


// ----------------- UNITY INTEGRATION -----------------
// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
	switch (eventType)
	{
	case kUnityGfxDeviceEventInitialize:
	case kUnityGfxDeviceEventAfterReset:
		gub_renderer = deviceType;
		gub_device = device;

		switch (gub_renderer)
		{
#if SUPPORT_D3D9
		case kUnityGfxRendererD3D9:
			gub_log("Set D3D9 graphics device");
			break;
#endif
#if SUPPORT_D3D11
		case kUnityGfxRendererD3D11:
			gub_log("Set D3D11 graphics device");
			break;
#endif
#if SUPPORT_OPENGL
		case kUnityGfxRendererOpenGL:
			gub_log("Set OpenGL graphics device");
			break;
#endif
#if SUPPORT_EGL
		case kUnityGfxRendererOpenGLES20:
		case kUnityGfxRendererOpenGLES30:
			gub_log("Set OpenGL-ES graphics device");
			break;
#endif
		default:
			gub_log("Unsupported graphic device %d", deviceType);
			break;
		}
		break;
	}
}

void gub_copy_texture(const char *data, int w, int h, void *native_texture_ptr)
{
	switch (gub_renderer)
	{
#if SUPPORT_D3D9
	case kUnityGfxRendererD3D9:
		gub_copy_texture_D3D9(data, w, h, native_texture_ptr);
		break;
#endif
#if SUPPORT_D3D11
	case kUnityGfxRendererD3D11:
		gub_copy_texture_D3D11(data, w, h, native_texture_ptr);
		break;
#endif
#if SUPPORT_OPENGL
	case kUnityGfxRendererOpenGL:
		gub_copy_texture_OpenGL(data, w, h, native_texture_ptr);
		break;
#endif
#if SUPPORT_EGL
	case kUnityGfxRendererOpenGLES20:
	case kUnityGfxRendererOpenGLES30:
		gub_copy_texture_EGL(data, w, h, native_texture_ptr);
		break;
#endif
	default:
		break;
	}
}

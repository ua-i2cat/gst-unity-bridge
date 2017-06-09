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
#include "gub_graphics.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#define SUPPORT_OPENGL 0
#define SUPPORT_D3D9 1
#define SUPPORT_D3D11 1
#elif defined(__ANDROID__)
#define SUPPORT_EGL 1
#else
#define SUPPORT_OPENGL 1
#endif

/* Created by Unity when the rendering engine is selected */
typedef void GUBGraphicDevice;

typedef GUBGraphicDevice* (*GUBCreateGraphicDevicePFN)(void* device, int deviceType);
typedef void(*GUBDestroyGraphicDevicePFN)(GUBGraphicDevice *gdevice);
typedef GUBGraphicContext* (*GUBCreateGraphicContextPFN)(GstPipeline *pipeline, float crop_x, float crop_y, float crop_width, float crop_height);
typedef GstContext* (*GUBProvideGraphicContextPFN)(GUBGraphicContext *gcontext, const gchar *type);
typedef void(*GUBDestroyGraphicContextPFN)(GUBGraphicContext *gcontext);
typedef void(*GUBCopyTexturePFN)(GUBGraphicContext *gcontext, GstVideoInfo *video_info, GstBuffer *buffer, void *native_texture_ptr);
typedef const gchar* (*GUBGetVideoBranchDescriptionPFN)();

typedef struct _GUBGraphicBackend {
    GUBCreateGraphicDevicePFN create_graphic_device;
    GUBDestroyGraphicDevicePFN destroy_graphic_device;
    GUBCreateGraphicContextPFN create_graphic_context;
    GUBProvideGraphicContextPFN provide_graphic_context;
    GUBDestroyGraphicContextPFN destroy_graphic_context;
    GUBCopyTexturePFN copy_texture;
    GUBGetVideoBranchDescriptionPFN get_video_branch_description;
} GUBGraphicBackend;

GUBGraphicBackend *gub_graphic_backend = NULL;
GUBGraphicDevice *gub_graphic_device = NULL;

#if SUPPORT_D3D9
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- D3D9 SUPPORT ---------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
#include <d3d9.h>

typedef struct _GUBGraphicContextD3D9 {
    float crop_left;
    float crop_top;
    float crop_right;
    float crop_bottom;
} GUBGraphicContextD3D9;

static GUBGraphicDevice *gub_create_graphic_context_d3d9(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom)
{
    GUBGraphicContextD3D9 *gcontext = (GUBGraphicContextD3D9 *)malloc(sizeof(GUBGraphicContextD3D9));
    gcontext->crop_left = crop_left;
    gcontext->crop_top = crop_top;
    gcontext->crop_right = crop_right;
    gcontext->crop_bottom = crop_bottom;
    return gcontext;
}

static void gub_destroy_graphic_context_d3d9(GUBGraphicContextD3D9 *gcontext)
{
    free(gcontext);
}

static void gub_copy_texture_d3d9(GUBGraphicContextD3D9 *gcontext, GstVideoInfo *video_info, GstBuffer *buffer, void *native_texture_ptr)
{
    static const GUID GUB_IID_IDirect3DTexture9 = { 0x85c31227, 0x3de5, 0x4f00, 0x9b, 0x3a, 0xf1, 0x1a, 0xc3, 0x8c, 0x18, 0xb5 };

    if (native_texture_ptr)
    {
        void *d3d_interface;
        IDirect3DTexture9* d3dtex = (IDirect3DTexture9*)native_texture_ptr;
        D3DLOCKED_RECT lr;
        GstVideoFrame video_frame;

        if (d3dtex->lpVtbl->QueryInterface(d3dtex, &GUB_IID_IDirect3DTexture9, &d3d_interface) != S_OK) {
            // This is not D3D9, we are probably inside the Editor in D3D11 mode and we assumed wrongly
            gub_log("I assumed this was D3D9 but it is not. Are you using the Unity Editor without -force-d3d9 ?");
            return;
        }
        d3dtex->lpVtbl->Release(d3dtex);
        if (d3dtex->lpVtbl->LockRect(d3dtex, 0, &lr, NULL, D3DLOCK_DISCARD) != D3D_OK)
            gub_log("Problem locking D3D texture");
        gst_video_frame_map(&video_frame, video_info, buffer, GST_MAP_READ);
        if (gcontext->crop_left == 0 && gcontext->crop_top == 0 && gcontext->crop_right == 0 && gcontext->crop_bottom == 0) {
            // No cropping
            memcpy((char*)lr.pBits, GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0), video_info->width * video_info->height * 4);
        }
        else {
            // Cropping
            int left = (int)(video_info->width  * gcontext->crop_left);
            int top = (int)(video_info->height * gcontext->crop_top);
            int width = (int)(video_info->width  * (1 - gcontext->crop_left - gcontext->crop_right));
            int height = (int)(video_info->height * (1 - gcontext->crop_top - gcontext->crop_bottom));
            char *dst_ptr = (char*)lr.pBits;
            char *src_ptr = (char *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0) + (top * video_info->width + left) * 4;
            int y;

            for (y = 0; y < height; y++, dst_ptr += lr.Pitch, src_ptr += video_info->width * 4) {
                memcpy(dst_ptr, src_ptr, width * 4);
            }
        }
        gst_video_frame_unmap(&video_frame);
        if (d3dtex->lpVtbl->UnlockRect(d3dtex, 0) != D3D_OK)
            gub_log("Problem unlocking D3D texture");
    }
}

static const gchar *gub_get_video_branch_description_d3d9()
{
    return "videoconvert ! video/x-raw,format=BGRA ! fakesink sync=1 qos=1 name=sink";
}

GUBGraphicBackend gub_graphic_backend_d3d9 = {
    /* create_graphic_device   */      NULL,
    /* destroy_graphic_device  */      NULL,
    /* create_graphic_context  */      (GUBCreateGraphicContextPFN)gub_create_graphic_context_d3d9,
    /* provide_graphic_context */      NULL,
    /* destroy_graphic_context */      (GUBDestroyGraphicContextPFN)gub_destroy_graphic_context_d3d9,
    /* copy_texture */                 (GUBCopyTexturePFN)gub_copy_texture_d3d9,
    /* get_video_branch_description */ (GUBGetVideoBranchDescriptionPFN)gub_get_video_branch_description_d3d9
};

#endif

#if SUPPORT_D3D11
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- D3D11 SUPPORT --------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
#include <d3d11.h>

typedef struct _GUBGraphicDeviceD3D11 {
    ID3D11Device* d3d11device;
} GUBGraphicDeviceD3D11;

typedef struct _GUBGraphicContextD3D11 {
    float crop_left;
    float crop_top;
    float crop_right;
    float crop_bottom;
    GstPipeline *pipeline;
    gboolean crop_setup;
} GUBGraphicContextD3D11;

static GUBGraphicDevice *gub_create_graphic_context_d3d11(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom)
{
    GUBGraphicContextD3D11 *gcontext = (GUBGraphicContextD3D11 *)malloc(sizeof(GUBGraphicContextD3D11));
    gcontext->crop_left = crop_left;
    gcontext->crop_top = crop_top;
    gcontext->crop_right = crop_right;
    gcontext->crop_bottom = crop_bottom;
    gcontext->crop_setup = FALSE;
    gcontext->pipeline = pipeline;
    return gcontext;
}

static void gub_destroy_graphic_context_d3d11(GUBGraphicContextD3D11 *gcontext)
{
    free(gcontext);
}

static void gub_copy_texture_d3d11(GUBGraphicContextD3D11 *gcontext, GstVideoInfo *video_info, GstBuffer *buffer, void *native_texture_ptr)
{
    GUBGraphicDeviceD3D11* gdevice = (GUBGraphicDeviceD3D11*)gub_graphic_device;
    ID3D11DeviceContext* ctx = NULL;
    gdevice->d3d11device->lpVtbl->GetImmediateContext(gdevice->d3d11device, &ctx);
    if (!gcontext->crop_setup) {
        GstElement *crop;
        crop = gst_bin_get_by_name(GST_BIN(gcontext->pipeline), "crop");
        if (crop) {
            g_object_set(crop,
                "left", (int)(video_info->width *  gcontext->crop_left),
                "top", (int)(video_info->height * gcontext->crop_top),
                "right", (int)(video_info->width *  gcontext->crop_right),
                "bottom", (int)(video_info->height * gcontext->crop_bottom),
                NULL);
            gst_object_unref(crop);
        }
        gcontext->crop_setup = TRUE;
    }
    // update native texture from code
    if (native_texture_ptr) {
        GstVideoFrame video_frame;
        gst_video_frame_map(&video_frame, video_info, buffer, GST_MAP_READ);
        ctx->lpVtbl->UpdateSubresource(ctx, (ID3D11Resource *)native_texture_ptr, 0, NULL, GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0), video_info->width * 4, 0);
        gst_video_frame_unmap(&video_frame);
    }
    ctx->lpVtbl->Release(ctx);
}

static GUBGraphicDevice *gub_create_graphic_device_d3d11(void* device, int deviceType)
{
    GUBGraphicDeviceD3D11 *gdevice = (GUBGraphicDeviceD3D11 *)malloc(sizeof(GUBGraphicDeviceD3D11));
    gdevice->d3d11device = (ID3D11Device*)device;
    return gdevice;
}

static void gub_destroy_graphic_device_d3d11(GUBGraphicDeviceD3D11 *gdevice)
{
    free(gdevice);
}

static const gchar *gub_get_video_branch_description_d3d11()
{
    return "videoconvert ! video/x-raw,format=RGBA ! videocrop name=crop ! fakesink sync=1 qos=1 name=sink";
}

GUBGraphicBackend gub_graphic_backend_d3d11 = {
    /* create_graphic_device   */      (GUBCreateGraphicDevicePFN)gub_create_graphic_device_d3d11,
    /* destroy_graphic_device  */      (GUBDestroyGraphicDevicePFN)gub_destroy_graphic_device_d3d11,
    /* create_graphic_context  */      (GUBCreateGraphicContextPFN)gub_create_graphic_context_d3d11,
    /* provide_graphic_context */      NULL,
    /* destroy_graphic_context */      (GUBDestroyGraphicContextPFN)gub_destroy_graphic_context_d3d11,
    /* copy_texture */                 (GUBCopyTexturePFN)gub_copy_texture_d3d11,
    /* get_video_branch_description */ (GUBGetVideoBranchDescriptionPFN)gub_get_video_branch_description_d3d11
};

#endif

#if SUPPORT_OPENGL
// --------------------------------------------------------------------------------------------------------------------
// -------------------------------------------------- OPENGL SUPPORT --------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglcontext.h>

#if GST_GL_HAVE_PLATFORM_WGL
#define GUB_GL_PLATFORM GST_GL_PLATFORM_WGL
#elif GST_GL_HAVE_PLATFORM_GLX
#define GUB_GL_PLATFORM GST_GL_PLATFORM_GLX
#else
#error "Unsupport GST_GL_PLATFORM"
#endif

typedef struct _GUBGraphicContextOpenGL {
    GstGLDisplay *display;
} GUBGraphicContextOpenGL;

static void gub_copy_texture_opengl(GUBGraphicContext *gcontext, GstVideoInfo *video_info, GstBuffer *buffer, void *native_texture_ptr)
{
    if (native_texture_ptr)
    {
        GLuint gltex = (GLuint)(size_t)(native_texture_ptr);
        GstVideoFrame video_frame;
        glBindTexture(GL_TEXTURE_2D, gltex);
        gst_video_frame_map(&video_frame, video_info, buffer, GST_MAP_READ);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, video_info->width, video_info->height, GL_RGB, GL_UNSIGNED_BYTE, GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0));
        gst_video_frame_unmap(&video_frame);
    }
}

static GUBGraphicContext *gub_create_graphic_context_opengl(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom)
{
    GUBGraphicContextOpenGL *gcontext = NULL;
    guintptr raw_context = gst_gl_context_get_current_gl_context(GUB_GL_PLATFORM);
    if (raw_context) {
        GstGLDisplay *display = gst_gl_display_new();
        GstGLContext *gl_context = gst_gl_context_new_wrapped(display, raw_context, GUB_GL_PLATFORM, GST_GL_API_OPENGL);

        gcontext = (GUBGraphicContextOpenGL *)malloc(sizeof(GUBGraphicContextOpenGL));
        gcontext->display = display;
    }
    else {
        gub_log("Could not retrieve current GL context");
    }

    return gcontext;
}

static void gub_destroy_graphic_context_opengl(GUBGraphicContextOpenGL *gcontext)
{
    if (gcontext) {
        if (gcontext->display) {
            gst_object_unref(gcontext->display);
        }
        free(gcontext);
    }
}

static const gchar *gub_get_video_branch_description_opengl()
{
    return "videoconvert ! video/x-raw,format=RGB ! fakesink sync=1 qos=1 name=sink";
}

GUBGraphicBackend gub_graphic_backend_opengl = {
    /* create_graphic_device   */      NULL,
    /* destroy_graphic_device  */      NULL,
    /* create_graphic_context  */      (GUBCreateGraphicContextPFN)gub_create_graphic_context_opengl,
    /* provide_graphic_context */      NULL,
    /* destroy_graphic_context */      (GUBDestroyGraphicContextPFN)gub_destroy_graphic_context_opengl,
    /* copy_texture */                 (GUBCopyTexturePFN)gub_copy_texture_opengl,
    /* get_video_branch_description */ (GUBGetVideoBranchDescriptionPFN)gub_get_video_branch_description_opengl
};

#endif

#if SUPPORT_EGL
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- EGL SUPPORT ----------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

#define GST_USE_UNSTABLE_API
#include <gst/gl/gstglcontext.h>

typedef struct _GUBGraphicContextEGL {
    GstGLContext *gl;
    GstGLDisplay *display;
    GLuint fbo;
    GLuint po;
    GLuint vao;
    GLuint vbo;
    GLint samplerLoc;
    float crop_left;
    float crop_top;
    float crop_right;
    float crop_bottom;
} GUBGraphicContextEGL;

static GLuint gub_load_shader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);

    if (shader == 0)
        return 0;

    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);

    // Compile the shader
    glCompileShader(shader);

    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            gub_log("Error compiling shader: %s", infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static int gub_create_program()
{
    GLbyte vShaderStr[] =
        "attribute vec4 aPosition;    \n"
        "attribute vec2 aTexCoord;    \n"
        "varying vec2 vTexCoord;      \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = aPosition;  \n"
        "   vTexCoord = aTexCoord;    \n"
        "}                            \n";

    GLbyte fShaderStr[] =
        "precision mediump float;                          \n"
        "varying vec2 vTexCoord;                           \n"
        "uniform sampler2D sTexture;                       \n"
        "void main()                                       \n"
        "{                                                 \n"
        "  gl_FragColor = texture2D(sTexture, vTexCoord);  \n"
        "}                                                 \n";

    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = gub_load_shader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = gub_load_shader(GL_FRAGMENT_SHADER, fShaderStr);

    // Create the program object
    programObject = glCreateProgram();

    if (programObject == 0)
        return 0;

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);

    // Bind aPosition to attribute 0 and aTexCoord to attribute 1
    glBindAttribLocation(programObject, 0, "aPosition");
    glBindAttribLocation(programObject, 1, "aTexCoord");

    // Link the program
    glLinkProgram(programObject);

    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
            gub_log("Error linking program: %s", infoLog);
            free(infoLog);
        }
        glDeleteProgram(programObject);
        return 0;
    }

    // Store the program object
    return programObject;
}

static void gub_copy_texture_egl(GUBGraphicContextEGL *gcontext, GstVideoInfo *video_info, GstBuffer *buffer, void *native_texture_ptr)
{
    if (!gcontext) return;

    if (native_texture_ptr)
    {
        GLint previous_vp[4];
        GLint previous_prog;
        GLint previous_fbo;
        GLint previous_tex;
        GLint previous_ab;
        GLint previous_rbo;
        GLint previous_vaenabled[2];

        GLenum status;
        GLuint unity_tex = (GLuint)(size_t)(native_texture_ptr);

        GstVideoFrame video_frame;
        GLuint gst_tex;
        gst_video_frame_map(&video_frame, video_info, buffer, GST_MAP_READ | GST_MAP_GL);
        gst_tex = *(guint *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0);

        glGetIntegerv(GL_VIEWPORT, previous_vp);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previous_prog);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_fbo);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_tex);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previous_ab);
        glGetIntegerv(GL_RENDERBUFFER_BINDING, &previous_rbo);
        glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &previous_vaenabled[0]);
        glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &previous_vaenabled[1]);

        glBindFramebuffer(GL_FRAMEBUFFER, gcontext->fbo);
        glViewport(
            -video_info->width * gcontext->crop_left,
            -video_info->height * gcontext->crop_top,
            video_info->width, video_info->height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, unity_tex, 0);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            gub_log("Frame buffer not complete, status 0x%x, unity_tex %d", status, unity_tex);
        }

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glPolygonOffset(0.0f, 0.0f);
        glDisable(GL_POLYGON_OFFSET_FILL);

        glUseProgram(gcontext->po);
        if (gcontext->gl->gl_vtable->BindVertexArray)
            gcontext->gl->gl_vtable->BindVertexArray(gcontext->vao);
        glBindBuffer(GL_ARRAY_BUFFER, gcontext->vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const GLvoid *)(0));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (const GLvoid *)(2 * sizeof(GLfloat)));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gst_tex);
        glUniform1i(gcontext->samplerLoc, 0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindFramebuffer(GL_FRAMEBUFFER, previous_fbo);
        glViewport(previous_vp[0], previous_vp[1], previous_vp[2], previous_vp[3]);
        if (gcontext->gl->gl_vtable->BindVertexArray)
            gcontext->gl->gl_vtable->BindVertexArray(0);
        glUseProgram(previous_prog);
        glBindBuffer(GL_ARRAY_BUFFER, previous_ab);
        glBindRenderbuffer(GL_RENDERBUFFER, previous_rbo);
        if (!previous_vaenabled[0])
            glDisableVertexAttribArray(0);
        if (!previous_vaenabled[1])
            glDisableVertexAttribArray(1);
        glBindTexture(GL_TEXTURE_2D, previous_tex);

        gst_video_frame_unmap(&video_frame);
    }
}

static GUBGraphicContext *gub_create_graphic_context_egl(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom)
{
    static const GLfloat vVertices[] = {
        -1.f, -1.f,   0.f, 0.f,
        -1.f,  1.f,   0.f, 1.f,
         1.f, -1.f,   1.f, 0.f,
         1.f,  1.f,   1.f, 1.f
    };
    guintptr raw_context;
    GstStructure *s;
    GstGLDisplay *display;
    GstGLContext *gl_context;
    GUBGraphicContextEGL *gcontext;

    raw_context = gst_gl_context_get_current_gl_context(GST_GL_PLATFORM_EGL);
    if (!raw_context) {
        gub_log("Could not retrieve current EGL context");
        return NULL;
    }

    display = (GstGLDisplay *)gst_gl_display_egl_new();
    gl_context = gst_gl_context_new_wrapped(display, raw_context, GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);
    gub_log("Current GL context is %p (GSTGL Platform %s GSTGL API %s)", raw_context,
        gst_gl_platform_to_string(gst_gl_context_get_gl_platform(gl_context)),
        gst_gl_api_to_string(gst_gl_context_get_gl_api(gl_context)));
    gub_log("VENDOR: %s", glGetString(GL_VENDOR));
    gub_log("RENDERER: %s", glGetString(GL_RENDERER));
    gub_log("VERSION: %s", glGetString(GL_VERSION));
    gub_log("GLSL VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    gcontext = (GUBGraphicContextEGL *)malloc(sizeof(GUBGraphicContextEGL));
    gcontext->gl = gl_context;
    gcontext->display = display;
    gcontext->crop_left = crop_left;
    gcontext->crop_top = crop_top;
    gcontext->crop_right = crop_right;
    gcontext->crop_bottom = crop_bottom;

    glGenFramebuffers(1, &gcontext->fbo);

    if (gl_context->gl_vtable->GenVertexArrays)
        gl_context->gl_vtable->GenVertexArrays(1, &gcontext->vao);
    if (gcontext->gl->gl_vtable->BindVertexArray)
        gcontext->gl->gl_vtable->BindVertexArray(gcontext->vao);

    glGenBuffers(1, &gcontext->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gcontext->vbo);
    glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), vVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    gcontext->po = gub_create_program();

    gcontext->samplerLoc = glGetUniformLocation(gcontext->po, "sTexture");

    return gcontext;
}

static GstContext *gub_provide_graphic_context_egl(GUBGraphicContextEGL *gcontext, const gchar *type)
{
    GstContext *context = NULL;

    gub_log("Providing context. gub_context=%p, type=%s", gcontext, type);

    if (!gcontext) return NULL;

    if (type != NULL) {
        if (g_strcmp0(type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
            context = gst_context_new(GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
            gst_context_set_gl_display(context, gcontext->display);
        }
        else if (g_strcmp0(type, "gst.gl.app_context") == 0) {
            GstStructure *s;
            context = gst_context_new("gst.gl.app_context", TRUE);
            s = gst_context_writable_structure(context);
            gst_structure_set(s, "context", GST_GL_TYPE_CONTEXT, gcontext->gl, NULL);
        }
        else if (g_strcmp0(type, "gst.gl.local_context") == 0) {
            GstGLContext *local_context = gst_gl_context_new(gcontext->display);
            GError *error = NULL;
            gst_gl_context_create(local_context, gcontext->gl, &error);
            if (error) {
                gub_log("Cannot create local context: %s", error->message);
                g_error_free(error);
            }
            else {
                GstStructure *s;
                context = gst_context_new("gst.gl.local_context", TRUE);
                s = gst_context_writable_structure(context);
                gst_structure_set(s, "context", GST_GL_TYPE_CONTEXT, local_context, NULL);
            }
        }
    }
    return context;
}

static void gub_destroy_graphic_context_egl(GUBGraphicContextEGL *gcontext)
{
    if (gcontext) {
        if (gcontext->display) {
            gst_object_unref(gcontext->display);
        }
        glDeleteFramebuffers(1, &gcontext->fbo);
        glDeleteProgram(gcontext->po);
        if (gcontext->gl->gl_vtable->DeleteVertexArrays)
            gcontext->gl->gl_vtable->DeleteVertexArrays(1, &gcontext->vao);
        glDeleteBuffers(1, &gcontext->vbo);
        free(gcontext);
    }
}

static const gchar *gub_get_video_branch_description_egl()
{
    return "glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),texture-target=2D ! fakesink sync=1 qos=1 name=sink";
}

GUBGraphicBackend gub_graphic_backend_egl = {
    /* create_graphic_device   */      NULL,
    /* destroy_graphic_device  */      NULL,
    /* create_graphic_context  */      (GUBCreateGraphicContextPFN)gub_create_graphic_context_egl,
    /* provide_graphic_context */      (GUBProvideGraphicContextPFN)gub_provide_graphic_context_egl,
    /* destroy_graphic_context */      (GUBDestroyGraphicContextPFN)gub_destroy_graphic_context_egl,
    /* copy_texture */                 (GUBCopyTexturePFN)gub_copy_texture_egl,
    /* get_video_branch_description */ (GUBGetVideoBranchDescriptionPFN)gub_get_video_branch_description_egl
};

#endif

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------- Internal API ---------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

GUBGraphicContext *gub_create_graphic_context(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom)
{
    GUBGraphicContext *gcontext = NULL;
    if (gub_graphic_backend && gub_graphic_backend->create_graphic_context) {
        gcontext = gub_graphic_backend->create_graphic_context(pipeline, crop_left, crop_top, crop_right, crop_bottom);
    }
    return gcontext;
}

GstContext *gub_provide_graphic_context(GUBGraphicContext *gcontext, const gchar *type)
{
    GstContext *context = NULL;
    if (gub_graphic_backend && gub_graphic_backend->provide_graphic_context) {
        context = gub_graphic_backend->provide_graphic_context(gcontext, type);
    }
    return context;
}


void gub_destroy_graphic_context(GUBGraphicContext *gcontext)
{
    if (gub_graphic_backend && gub_graphic_backend->destroy_graphic_context) {
        gub_graphic_backend->destroy_graphic_context(gcontext);
    }
}

gboolean gub_blit_image(GUBGraphicContext *gcontext, GstSample *sample, void *texture_native_ptr)
{
    GstBuffer *buffer = NULL;
    GstCaps *caps = NULL;
    GstVideoInfo video_info;

    if (!gub_graphic_backend || !gub_graphic_backend->copy_texture) {
        return FALSE;
    }

    buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gub_log("Sample contains no buffer");
        return FALSE;
    }

    caps = gst_sample_get_caps(sample);
    gst_video_info_from_caps(&video_info, caps);

    gub_graphic_backend->copy_texture(gcontext, &video_info, buffer, texture_native_ptr);

    return TRUE;
}

const gchar *gub_get_video_branch_description()
{
    const gchar *description = NULL;

#if SUPPORT_D3D9
    if (!gub_graphic_backend) {
        // Not much more we can do... Unity is not calling UnitySetGraphicsDevice from within the Editor,
        // and all other backends require a Device or a Context to work.
        gub_log("No graphic backend defined. Assuming we are in the Unity Editor using DX9.");
        gub_graphic_backend = &gub_graphic_backend_d3d9;
    }
#endif

    if (gub_graphic_backend && gub_graphic_backend->get_video_branch_description) {
        description = gub_graphic_backend->get_video_branch_description();
    }
    return description;
}

// --------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------- Unity integration ------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// If exported by a plugin, this function will be called when graphics device is created, destroyed,
// and before and after it is reset (ie, resolution changed).
void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{
    switch (eventType)
    {
    case kUnityGfxDeviceEventInitialize:
    case kUnityGfxDeviceEventAfterReset:

        switch (deviceType)
        {
#if SUPPORT_D3D9
        case kUnityGfxRendererD3D9:
            gub_graphic_backend = &gub_graphic_backend_d3d9;
            gub_log("Set D3D9 graphic device");
            break;
#endif
#if SUPPORT_D3D11
        case kUnityGfxRendererD3D11:
            gub_graphic_backend = &gub_graphic_backend_d3d11;
            gub_log("Set D3D11 graphic device");
            break;
#endif
#if SUPPORT_OPENGL
        case kUnityGfxRendererOpenGL:
        case kUnityGfxRendererGLCore:
            gub_graphic_backend = &gub_graphic_backend_opengl;
            gub_log("Set OpenGL graphic device");
            break;
#endif
#if SUPPORT_EGL
        case kUnityGfxRendererOpenGLES20:
        case kUnityGfxRendererOpenGLES30:
            gub_graphic_backend = &gub_graphic_backend_egl;
            gub_log("Set OpenGL-ES graphics device");
            break;
#endif
        default:
            gub_log("Unsupported graphic device %d", deviceType);
            break;
        }
        if (gub_graphic_backend->create_graphic_device) {
            gub_graphic_device = gub_graphic_backend->create_graphic_device(device, deviceType);
        }
        break;
    case kUnityGfxDeviceEventShutdown:
    case kUnityGfxDeviceEventBeforeReset:
        if (gub_graphic_device && gub_graphic_backend && gub_graphic_backend->destroy_graphic_device) {
            gub_log("Destroying graphic device");
            gub_graphic_backend->destroy_graphic_device(gub_graphic_device);
        }
        break;
    }
}

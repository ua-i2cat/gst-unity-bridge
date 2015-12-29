/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
 */

#ifndef __GUB_H__
#define __GUB_H__

#include <gst/gst.h>

#if _WIN32
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

/* Created from the gst pipeline */
typedef void GUBGraphicContext;

GUBGraphicContext *gub_create_graphic_context(GstElement *pipeline);
void gub_destroy_graphic_context(GUBGraphicContext *context);
void gub_copy_texture(GUBGraphicContext *gcontext, const char *data, int w, int h, void *_TextureNativePtr);

void gub_log(const char *format, ...);

#endif

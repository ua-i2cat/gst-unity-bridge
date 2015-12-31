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
gboolean gub_blit_image(GUBGraphicContext *gcontext, GstSample *sample, void *texture_native_ptr);
const gchar *gub_get_video_branch_description();

void gub_log(const char *format, ...);

#endif

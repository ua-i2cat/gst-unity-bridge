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

GUBGraphicContext *gub_create_graphic_context(GstPipeline *pipeline, float crop_left, float crop_top, float crop_right, float crop_bottom);
GstContext *gub_provide_graphic_context(GUBGraphicContext *gcontext, const gchar *type);
void gub_destroy_graphic_context(GUBGraphicContext *context);
gboolean gub_blit_image(GUBGraphicContext *gcontext, GstSample *sample, void *texture_native_ptr);
const gchar *gub_get_video_branch_description();

void gub_log(const char *format, ...);
void gub_log_error(const char *message);

#endif

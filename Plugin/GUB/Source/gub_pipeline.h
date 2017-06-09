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

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include "gub.h"

typedef struct _GUBPipeline GUBPipeline;

typedef void(*GUBPipelineOnEosPFN)(GUBPipeline *userdata);
typedef void(*GUBPipelineOnErrorPFN)(GUBPipeline *userdata, char *message);
typedef void(*GUBPipelineOnQosPFN)(GUBPipeline *userdata,
    gint64 current_jitter, guint64 current_running_time, guint64 current_stream_time, guint64 current_timestamp,
    gdouble proportion, guint64 processed, guint64 dropped);

void gub_log_pipeline(GUBPipeline *pipeline, const char *format, ...);

EXPORT_API void *gub_pipeline_create(const char *name,
    GUBPipelineOnEosPFN eos_handler, GUBPipelineOnErrorPFN error_handler, GUBPipelineOnQosPFN qos_handler,
    void *userdata);

EXPORT_API void gub_pipeline_close(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_destroy(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_play(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_pause(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_stop(GUBPipeline *pipeline);

EXPORT_API gint32 gub_pipeline_is_loaded(GUBPipeline *pipeline);

EXPORT_API gint32 gub_pipeline_is_playing(GUBPipeline *pipeline);

EXPORT_API double gub_pipeline_get_duration(GUBPipeline *pipeline);

EXPORT_API double gub_pipeline_get_position(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_set_position(GUBPipeline *pipeline, double position);

EXPORT_API void gub_pipeline_setup_decoding_clock(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index,
    const gchar *net_clock_addr, int net_clock_port, guint64 basetime,
    float crop_left, float crop_top, float crop_right, float crop_bottom, gboolean isDvbWc);

EXPORT_API void gub_pipeline_setup_decoding(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index,
	const gchar *net_clock_addr, int net_clock_port, guint64 basetime,
	float crop_left, float crop_top, float crop_right, float crop_bottom);

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height);

EXPORT_API void gub_pipeline_blit_image(GUBPipeline *pipeline, void *_TextureNativePtr);

EXPORT_API void gub_pipeline_setup_encoding(GUBPipeline *pipeline, const gchar *filename,
    int width, int height);

EXPORT_API void gub_pipeline_consume_image(GUBPipeline *pipeline, guint8 *rawdata, int size);

EXPORT_API void gub_pipeline_stop_encoding(GUBPipeline *pipeline);

EXPORT_API void gub_pipeline_set_volume(GUBPipeline *pipeline, gdouble volume);

EXPORT_API void gub_pipeline_set_adaptive_bitrate_limit(GUBPipeline *pipeline, gfloat bitrate_limit);

EXPORT_API void gub_pipeline_set_basetime(GUBPipeline *pipeline, guint64 basetime);

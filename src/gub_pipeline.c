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
#include <gst/video/video.h>
#include <gst/net/gstnet.h>
#include <memory.h>
#include <stdio.h>
#include "gub.h"

#define MAX_JITTERBUFFER_DELAY_MS 40
#define MAX_PIPELINE_DELAY_MS 500

typedef struct _GUBPipeline {
	GUBGraphicContext *graphic_context;
	gboolean supports_cropping_blit;

	GstElement *pipeline;
	GstSample *last_sample;
	GstClock *net_clock;
	gboolean playing;
	gboolean play_requested;
	int video_index;
	int audio_index;
	float video_crop_left;
	float video_crop_top;
	float video_crop_right;
	float video_crop_bottom;
} GUBPipeline;

void gub_log_pipeline(GUBPipeline *pipeline, const char *format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	gchar *new_format = g_strdup_printf("[pipeline 0x%p] %s", pipeline, format);
	gchar *final_string = g_strdup_vprintf(new_format, argptr);
	g_free(new_format);
	va_end(argptr);
	gub_log(final_string);
	g_free(final_string);
}

EXPORT_API GUBPipeline *gub_pipeline_create()
{
	GUBPipeline *pipeline = (GUBPipeline *)malloc(sizeof(GUBPipeline));
	memset(pipeline, 0, sizeof(GUBPipeline));
	return pipeline;
}

EXPORT_API void gub_pipeline_close(GUBPipeline *pipeline)
{
	gub_destroy_graphic_context(pipeline->graphic_context);
	if (pipeline->pipeline) {
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_NULL);
	}
	if (pipeline->last_sample) {
		gst_sample_unref(pipeline->last_sample);
	}
	memset(pipeline, 0, sizeof(GUBPipeline));
}

EXPORT_API void gub_pipeline_destroy(GUBPipeline *pipeline)
{
	gub_pipeline_close(pipeline);
	free(pipeline);
}

EXPORT_API void gub_pipeline_play(GUBPipeline *pipeline)
{
	/* We cannot start playing immediately. This might be called on Script Start(), and,
	on Android, the application has not yet provided a GL context at that point.
	Instead, we will start the pipeline from the grab_frame method, which is called
	when the app has initialized GL (Script OnGui). */
	pipeline->play_requested = TRUE;
}

EXPORT_API void gub_pipeline_pause(GUBPipeline *pipeline)
{
	if (pipeline->pipeline) {
		gub_log_pipeline(pipeline, "Setting pipeline to PAUSED");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PAUSED);
		gub_log_pipeline(pipeline, "State change completed");
		pipeline->play_requested = FALSE;
		pipeline->playing = FALSE;
	}
}

EXPORT_API void gub_pipeline_stop(GUBPipeline *pipeline)
{
	if (pipeline->pipeline) {
		gub_log_pipeline(pipeline, "Setting pipeline to NULL");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_NULL);
		gub_log_pipeline(pipeline, "State change completed");
	}
}

EXPORT_API gint32 gub_pipeline_is_loaded(GUBPipeline *pipeline)
{
	return (pipeline->pipeline != NULL);
}

EXPORT_API gint32 gub_pipeline_is_playing(GUBPipeline *pipeline)
{
	return pipeline->playing;
}

static gboolean select_stream (GstBin *rtspsrc, guint num, GstCaps *caps, GUBPipeline *pipeline)
{
	gboolean select = (num == pipeline->video_index || num == pipeline->audio_index);
	gchar *caps_str = gst_caps_to_string(caps);
	gub_log_pipeline(pipeline, "Found stream #%d (%s): %s", num, select ? "SELECTED" : "IGNORED", caps_str);
	g_free(caps_str);
	return select;
}

static void source_created(GstBin *playbin, GstElement *source, GUBPipeline *pipeline)
{
	gub_log_pipeline(pipeline, "Setting properties to source %s", gst_plugin_feature_get_name(gst_element_get_factory(source)));
	g_object_set(source, "latency", MAX_JITTERBUFFER_DELAY_MS, NULL);
	g_object_set(source, "ntp-time-source", 3, NULL);
	g_object_set(source, "buffer-mode", 4, NULL);
	g_object_set(source, "ntp-sync", TRUE, NULL);
//	g_object_set(source, "protocols", 4, NULL);

	g_signal_connect(source, "select-stream", G_CALLBACK(select_stream), pipeline);
}

static GstPadProbeReturn pad_probe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
	GUBPipeline *pipeline = (GUBPipeline *)user_data;
	GstPadProbeReturn ret = GST_PAD_PROBE_OK;
	GstQuery *query;
	const gchar *context_type = NULL;
	GError *error = NULL;
	GstContext *context = NULL;

	if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM) == 0) goto beach;

	query = GST_PAD_PROBE_INFO_QUERY(info);
	if (GST_QUERY_TYPE(query) != GST_QUERY_CONTEXT) goto beach;

	gst_query_parse_context_type(query, &context_type);

	context = gub_provide_graphic_context(pipeline->graphic_context, context_type);
	if (context) {
		gst_query_set_context(query, context);
		ret = GST_PAD_PROBE_HANDLED;

		gub_log_pipeline(pipeline, "Query for context type %s answered with context %p", context_type, context);
		gst_context_unref(context);
	}
beach:
	return ret;
}

EXPORT_API void gub_pipeline_setup(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index,
	const gchar *net_clock_addr, int net_clock_port,
	float crop_left, float crop_top, float crop_right, float crop_bottom)
{
	GError *err = NULL;
	GstElement *vsink;
	gchar *full_pipeline_description = NULL;
	GstBus *bus = NULL;

	if (pipeline->pipeline) {
		gub_pipeline_close(pipeline);
	}

	full_pipeline_description = g_strdup_printf("playbin uri=%s", uri);
	gub_log_pipeline(pipeline, "Using pipeline: %s", full_pipeline_description);

	pipeline->pipeline = gst_parse_launch(full_pipeline_description, &err);
	g_free(full_pipeline_description);
	if (err) {
		gub_log_pipeline(pipeline, "Failed to create pipeline: %s", err->message);
		return;
	}

	vsink = gst_parse_bin_from_description(gub_get_video_branch_description(), TRUE, NULL);
	gub_log_pipeline(pipeline, "Using video sink: %s", gub_get_video_branch_description());
	g_object_set(pipeline->pipeline, "video-sink", vsink, NULL);
	g_object_set(pipeline->pipeline, "flags", 0x0003, NULL);

	if (vsink) {
		// Plant a pad probe to answer context queries
		GstElement *sink;
		sink = gst_bin_get_by_name(GST_BIN(vsink), "sink");
		if (sink) {
			GstPad *pad = gst_element_get_static_pad(sink, "sink");
			if (pad) {
				gulong id = gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_QUERY_DOWNSTREAM, pad_probe, pipeline, NULL);
				gub_log_pipeline(pipeline, "Sink pad probe id is %d", id);
				gst_object_unref(pad);
			}
		}
	}

	// If the video branch does not have a "videocrop" element, we assume this graphic backend
	// is doing cropping during blitting.
	if (g_strstr_len(gub_get_video_branch_description(), -1, "videocrop") == NULL) {
		pipeline->supports_cropping_blit = TRUE;
	}
	gub_log_pipeline(pipeline, "Video branch %s cropping and blitting in one operation",
		pipeline->supports_cropping_blit ? "supports" : "does not support");

	pipeline->video_index = video_index;
	pipeline->audio_index = audio_index;

	// Only Enable cropping if it makes sense
	if (crop_left + crop_right < 1 && crop_top + crop_bottom < 1) {
		pipeline->video_crop_left = crop_left;
		pipeline->video_crop_top = crop_top;
		pipeline->video_crop_right = crop_right;
		pipeline->video_crop_bottom = crop_bottom;
	}

	g_signal_connect(pipeline->pipeline, "source-setup", G_CALLBACK(source_created), pipeline);

	if (net_clock_addr != NULL) {
		gint64 start, stop;
		gub_log_pipeline(pipeline, "Trying to synchronize to network clock at %s %d", net_clock_addr, net_clock_port);
		pipeline->net_clock = gst_net_client_clock_new("net_clock", net_clock_addr, net_clock_port, 0);
		if (!pipeline->net_clock) {
			gub_log_pipeline(pipeline, "Could not create network clock at %s %d", net_clock_addr, net_clock_port);
			return;
		}

		start = g_get_monotonic_time();
		gst_clock_wait_for_sync(pipeline->net_clock, 30 * GST_SECOND);
		stop = g_get_monotonic_time();
		if (gst_clock_is_synced(pipeline->net_clock)) {
			gub_log_pipeline(pipeline, "Synchronized to network clock in %g seconds", (stop - start) / 1e6);
		} else {
			gub_log_pipeline(pipeline, "Could not synchronize to network clock after %g seconds", (stop - start) / 1e6);
		}

		gst_pipeline_use_clock(GST_PIPELINE(pipeline->pipeline), pipeline->net_clock);
		gst_pipeline_set_latency(GST_PIPELINE(pipeline->pipeline), MAX_PIPELINE_DELAY_MS * GST_MSECOND);
	}
}

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height)
{
	GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline->pipeline), "sink");
	GstCaps *last_caps = NULL;
	GstVideoInfo info;

	if (!pipeline->graphic_context) {
		pipeline->graphic_context = gub_create_graphic_context(
			GST_PIPELINE(pipeline->pipeline),
			pipeline->video_crop_left, pipeline->video_crop_top, pipeline->video_crop_right, pipeline->video_crop_bottom);
	}

	if (pipeline->playing == FALSE && pipeline->play_requested == TRUE) {
		gub_log_pipeline(pipeline, "Setting pipeline to PLAYING");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PLAYING);
		gub_log_pipeline(pipeline, "State change completed");
		pipeline->playing = TRUE;
	}

	if (pipeline->last_sample) {
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
	}

	if (!sink) {
		gub_log_pipeline(pipeline, "Pipeline does not contain a sink named 'sink'");
		return 0;
	}

	g_object_get(sink, "last-sample", &pipeline->last_sample, NULL);
	gst_object_unref(sink);
	if (!pipeline->last_sample) {
		gub_log_pipeline(pipeline, "Could not read property 'last-sample' from sink %s",
			gst_plugin_feature_get_name(gst_element_get_factory(sink)));
		return 0;
	}

	last_caps = gst_sample_get_caps(pipeline->last_sample);
	if (!last_caps) {
		gub_log_pipeline(pipeline, "Sample contains no caps in sink %s",
			gst_plugin_feature_get_name(gst_element_get_factory(sink)));
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
		return 0;
	}

	gst_video_info_from_caps(&info, last_caps);

#if 0
	// Uncomment to have some timing debug information
	if (pipeline->net_clock) {
		GstBuffer *buff = gst_sample_get_buffer(pipeline->last_sample);
		GstClockTime pts = GST_BUFFER_PTS(buff);
		GstClockTime curr = gst_clock_get_time(pipeline->net_clock);
		GstClockTime base = gst_element_get_base_time(pipeline->pipeline);
		gub_log_pipeline(pipeline, "Buffer PTS is %" GST_TIME_FORMAT ", current is %" GST_TIME_FORMAT ", base is %" GST_TIME_FORMAT, GST_TIME_ARGS(pts), GST_TIME_ARGS(curr), GST_TIME_ARGS(base));
		if (gst_element_get_clock(pipeline->pipeline) != pipeline->net_clock)
			gub_log_pipeline(pipeline, "WRONG CLOCK: pipeline=%p net_clock=%p", gst_element_get_clock(pipeline->pipeline), pipeline->net_clock);
	}
#endif

	if (pipeline->supports_cropping_blit) {
		*width =  (int)(info.width  * (1 - pipeline->video_crop_left - pipeline->video_crop_right));
		*height = (int)(info.height * (1 - pipeline->video_crop_top - pipeline->video_crop_bottom));
	} else {
		*width = info.width;
		*height = info.height;
	}

	return 1;
}

EXPORT_API void gub_pipeline_blit_image(GUBPipeline *pipeline, void *_TextureNativePtr, int _UnityTextureWidth, int _UnityTextureHeight)
{
	if (!pipeline->last_sample) {
		return;
	}

	gub_blit_image(pipeline->graphic_context, pipeline->last_sample, _TextureNativePtr);

	gst_sample_unref(pipeline->last_sample);
	pipeline->last_sample = NULL;
}

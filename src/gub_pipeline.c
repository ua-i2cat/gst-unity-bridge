/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
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

	GstElement *pipeline;
	GstSample *last_sample;
	int last_width;
	int last_height;
	GstClock *net_clock;
	gboolean playing;
	gboolean play_requested;
	int video_index;
	int audio_index;
} GUBPipeline;

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
		gub_log("Setting pipeline to PAUSED");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PAUSED);
		gub_log("State change completed");
		pipeline->play_requested = FALSE;
		pipeline->playing = FALSE;
	}
}

EXPORT_API void gub_pipeline_stop(GUBPipeline *pipeline)
{
	if (pipeline->pipeline) {
		gub_log("Setting pipeline to NULL");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_NULL);
		gub_log("State change completed");
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
	gub_log("Found stream #%d (%s): %s", num, select ? "SELECTED" : "IGNORED", caps_str);
	g_free(caps_str);
	return select;
}

static void source_created(GstBin *playbin, GstElement *source, GUBPipeline *pipeline)
{
	gub_log("Setting properties to source %s", gst_plugin_feature_get_name(gst_element_get_factory(source)));
	g_object_set(source, "latency", MAX_JITTERBUFFER_DELAY_MS, NULL);
	g_object_set(source, "ntp-time-source", 3, NULL);
	g_object_set(source, "buffer-mode", 4, NULL);
	g_object_set(source, "ntp-sync", TRUE, NULL);
//	g_object_set(source, "protocols", 4, NULL);

	g_signal_connect(source, "select-stream", G_CALLBACK(select_stream), pipeline);
}

static gboolean sync_bus_call(GstBus *bus, GstMessage *msg, GUBPipeline *pipeline)
{
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_NEED_CONTEXT:
	{
		const gchar *context_type;
		GstContext *context = NULL;

		gst_message_parse_context_type(msg, &context_type);
		gub_log("got need context %s", context_type);
		gub_provide_graphic_context(pipeline->graphic_context, GST_ELEMENT(msg->src), context_type);

		break;
	}
	default:
		break;
	}

	return FALSE;
}

EXPORT_API void gub_pipeline_setup(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index, const gchar *net_clock_addr, int net_clock_port)
{
	GError *err = NULL;
	GstElement *vsink;
	gchar *full_pipeline_description = NULL;
	GstBus *bus = NULL;

	if (pipeline->pipeline) {
		gub_pipeline_close(pipeline);
	}

	full_pipeline_description = g_strdup_printf("playbin uri=%s", uri);
	gub_log("Using pipeline: %s", full_pipeline_description);

	pipeline->pipeline = gst_parse_launch(full_pipeline_description, &err);
	g_free(full_pipeline_description);
	if (err) {
		gub_log("Failed to create pipeline: %s", err->message);
		return;
	}

	vsink = gst_parse_bin_from_description(gub_get_video_branch_description(), TRUE, NULL);
	gub_log("Using video sink: %s", gub_get_video_branch_description());
	g_object_set(pipeline->pipeline, "video-sink", vsink, NULL);
	g_object_set(pipeline->pipeline, "flags", 0x0003, NULL);

	pipeline->video_index = video_index;
	pipeline->audio_index = audio_index;

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline->pipeline));
	gst_bus_enable_sync_message_emission(bus);
	g_signal_connect(bus, "sync-message", G_CALLBACK(sync_bus_call), pipeline);
	gst_object_unref(bus);

	g_signal_connect(pipeline->pipeline, "source-setup", G_CALLBACK(source_created), pipeline);

	if (net_clock_addr != NULL) {
		gint64 start, stop;
		gub_log("Trying to synchronize to network clock at %s %d", net_clock_addr, net_clock_port);
		pipeline->net_clock = gst_net_client_clock_new("net_clock", net_clock_addr, net_clock_port, 0);
		if (!pipeline->net_clock) {
			gub_log("Could not create network clock at %s %d", net_clock_addr, net_clock_port);
			return;
		}

		start = g_get_monotonic_time();
		gst_clock_wait_for_sync(pipeline->net_clock, 30 * GST_SECOND);
		stop = g_get_monotonic_time();
		if (gst_clock_is_synced(pipeline->net_clock)) {
			gub_log("Synchronized to network clock in %g seconds", (stop - start) / 1e6);
		} else {
			gub_log("Could not synchronize to network clock after %g seconds", (stop - start) / 1e6);
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
		pipeline->graphic_context = gub_create_graphic_context();
	}

	if (pipeline->playing == FALSE && pipeline->play_requested == TRUE) {
		gub_log("Setting pipeline to PLAYING");
		gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PLAYING);
		gub_log("State change completed");
		pipeline->playing = TRUE;
	}

	if (pipeline->last_sample) {
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
	}

	if (!sink) {
		gub_log("Pipeline does not contain a sink named 'sink'");
		return 0;
	}

	g_object_get(sink, "last-sample", &pipeline->last_sample, NULL);
	gst_object_unref(sink);
	if (!pipeline->last_sample) {
		gub_log("Could not read property 'last-sample' from sink %s",
			gst_plugin_feature_get_name(gst_element_get_factory(sink)));
		return 0;
	}

	last_caps = gst_sample_get_caps(pipeline->last_sample);
	if (!last_caps) {
		gub_log("Sample contains no caps in sink %s",
			gst_plugin_feature_get_name(gst_element_get_factory(sink)));
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
		return 0;
	}

	gst_video_info_from_caps(&info, last_caps);

	if (info.finfo->format != GST_VIDEO_FORMAT_RGB ||
		info.finfo->bits != 8) {
		gub_log("Buffer format is not RGB24 (it is %s) in sink %s",
			gst_video_format_to_string(info.finfo->format),
			gst_plugin_feature_get_name(gst_element_get_factory(sink)));
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
		return 0;
	}

#if 0
	if (pipeline->net_clock) {
		GstBuffer *buff = gst_sample_get_buffer(pipeline->last_sample);
		GstClockTime pts = GST_BUFFER_PTS(buff);
		GstClockTime curr = gst_clock_get_time(pipeline->net_clock) - gst_element_get_base_time(pipeline->pipeline);
		gub_log("Buffer PTS is %" GST_TIME_FORMAT " and current is %" GST_TIME_FORMAT, GST_TIME_ARGS(pts), GST_TIME_ARGS(curr));
	}
#endif

	pipeline->last_width = *width = info.width;
	pipeline->last_height = *height = info.height;

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

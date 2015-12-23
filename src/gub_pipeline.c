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
	GstElement *pipeline;
	GstSample *last_sample;
	int last_width;
	int last_height;
	GstClock *net_clock;
} GUBPipeline;

EXPORT_API GUBPipeline *gub_pipeline_create()
{
	GUBPipeline *pipeline = (GUBPipeline *)malloc(sizeof(GUBPipeline));
	memset(pipeline, 0, sizeof(GUBPipeline));
	return pipeline;
}

EXPORT_API void gub_pipeline_destroy(GUBPipeline *pipeline)
{
}

EXPORT_API void gub_pipeline_play(GUBPipeline *pipeline)
{
}

EXPORT_API void gub_pipeline_pause(GUBPipeline *pipeline)
{
}

EXPORT_API void gub_pipeline_stop(GUBPipeline *pipeline)
{
}

EXPORT_API gint32 gub_pipeline_is_loaded(GUBPipeline *pipeline)
{
	return FALSE;
}

EXPORT_API gint32 gub_pipeline_is_playing(GUBPipeline *pipeline)
{
	return FALSE;
}

EXPORT_API void gub_pipeline_close(GUBPipeline *pipeline)
{
}

static void source_created(GstElement *pipe, GstElement *source)
{
	gub_log("Setting properties to source %s", gst_plugin_feature_get_name(gst_element_get_factory(source)));
	g_object_set(source, "latency", MAX_JITTERBUFFER_DELAY_MS, NULL);
	g_object_set(source, "ntp-time-source", 3, NULL);
	g_object_set(source, "buffer-mode", 4, NULL);
	g_object_set(source, "ntp-sync", TRUE, NULL);
}

EXPORT_API void gub_pipeline_setup(GUBPipeline *pipeline, const gchar *pipeline_description, const gchar *net_clock_addr, int net_clock_port)
{
	GError *err = NULL;
	GstElement *source;

	pipeline->pipeline = gst_parse_launch(pipeline_description, &err);
	if (err) {
		gub_log("Failed to create pipeline: %s", err->message);
		return;
	}

	source = gst_bin_get_by_name(GST_BIN(pipeline->pipeline), "source");
	if (!source) {
		gub_log("Pipeline does not contain a source named 'source'");
		return;
	}
	g_signal_connect(source, "source-setup", G_CALLBACK(source_created), NULL);
	gst_object_unref(source);

	if (net_clock_addr != NULL) {
		gint64 start, stop;
		gub_log("Trying to synchronize to network clock at %s %d", net_clock_addr, net_clock_port);
		pipeline->net_clock = gst_net_client_clock_new("net_clock", net_clock_addr, net_clock_port, 0);
		if (!pipeline->net_clock) {
			gub_log("Could not create network clock at %s %d", net_clock_addr, net_clock_port);
			return;
		}

		start = g_get_monotonic_time();
		gst_clock_wait_for_sync(pipeline->net_clock, GST_CLOCK_TIME_NONE);
		stop = g_get_monotonic_time();
		gub_log("Synchronized to network clock in %g seconds", (stop - start) / 1e6);

		gst_pipeline_use_clock(GST_PIPELINE(pipeline->pipeline), pipeline->net_clock);
		gst_pipeline_set_latency(GST_PIPELINE(pipeline->pipeline), MAX_PIPELINE_DELAY_MS * GST_MSECOND);
	}

	gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PLAYING);
}

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height)
{
	GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline->pipeline), "sink");
	GstCaps *last_caps = NULL;
	GstVideoInfo info;

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
		gub_log("Could not read property 'last-sample' from sink");
		return 0;
	}

	last_caps = gst_sample_get_caps(pipeline->last_sample);
	if (!last_caps) {
		gub_log("Sample contains no caps");
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
		return 0;
	}

	gst_video_info_from_caps(&info, last_caps);

	if (info.finfo->format != GST_VIDEO_FORMAT_RGB ||
		info.finfo->bits != 8) {
		gub_log("Buffer format is not RGB24");
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
	GstBuffer *last_buffer = NULL;
	GstCaps *last_caps = NULL;
	GstMapInfo map;

	if (!pipeline->last_sample) {
		return;
	}

	last_buffer = gst_sample_get_buffer(pipeline->last_sample);
	if (!last_buffer) {
		gub_log("Sample contains no buffer");
		gst_sample_unref(pipeline->last_sample);
		pipeline->last_sample = NULL;
		return;
	}

	gst_buffer_map(last_buffer, &map, GST_MAP_READ);
	gub_copy_texture(map.data, pipeline->last_width, pipeline->last_height, _TextureNativePtr);
	gst_buffer_unmap(last_buffer, &map);

	gst_sample_unref(pipeline->last_sample);
	pipeline->last_sample = NULL;
}

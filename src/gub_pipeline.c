/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#include <gst/gst.h>
#include <gst/video/video.h>
#include <memory.h>
#include <stdio.h>
#include "gub.h"

typedef struct _GUBPipeline {
	GstElement *pipeline;
	GstSample *last_sample;
	int last_width;
	int last_height;
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

EXPORT_API void gub_pipeline_setup(GUBPipeline *pipeline, const gchar *pipeline_description)
{
	GError *err = NULL;
	pipeline->pipeline = gst_parse_launch(pipeline_description, &err);
	if (err) {
		gub_log("Failed to create pipeline: %s", err->message);
		return;
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

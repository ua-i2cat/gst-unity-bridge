/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#include <gst/gst.h>
#include <memory.h>
#include <stdio.h>
#include "gub.h"

typedef struct _GUBPipeline {
	GstElement *pipeline;
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
		fprintf(stderr, "GUB: Failed to create pipeline: %s", err->message);
		return;
	}
	gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_READY);
}

EXPORT_API void gub_pipeline_get_frame_size(GUBPipeline *pipeline, int *width, int *height)
{
}

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height)
{
	return TRUE;
}

EXPORT_API void gub_pipeline_blit_image(GUBPipeline *pipeline, void *_TextureNativePtr, int _UnityTextureWidth, int _UnityTextureHeight)
{
}


/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#include <gst/gst.h>
#include "gub.h"

typedef struct _GUBPipeline {
	int dummy;
} GUBPipeline;

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

EXPORT_API GUBPipeline *gub_pipeline_create()
{
	return NULL;
}

EXPORT_API void gub_pipeline_setup(GUBPipeline *pipeline, const gchar *pipeline_description)
{
}

EXPORT_API void gub_pipeline_get_frame_size(GUBPipeline *pipeline, int *width, int *height)
{
}

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height)
{
	return FALSE;
}

EXPORT_API void gub_pipeline_blit_image(GUBPipeline *pipeline, void *_TextureNativePtr, int _UnityTextureWidth, int _UnityTextureHeight)
{
}


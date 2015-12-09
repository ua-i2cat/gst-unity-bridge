/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#include <gst/gst.h>
#include "gub.h"

typedef void(*FuncPtr)(const char*);

EXPORT_API void gub_ref()
{
}

EXPORT_API void gub_unref()
{
}

EXPORT_API gint32 gub_is_active()
{
	return FALSE;
}

EXPORT_API void gub_set_debug_function(FuncPtr str)
{
}

/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#include <gst/gst.h>
#include "gub.h"
#include <stdio.h>

typedef void(*FuncPtr)(const char*);

gint gub_ref_count = 0;
GThread *gub_main_loop_thread = NULL;
GMainLoop *gub_main_loop = NULL;

gpointer gub_main_loop_func(gpointer data)
{
	fprintf(stdout, "GUB: Entering main loop\n");
	gub_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gub_main_loop);
	fprintf(stdout, "GUB: Quitting main loop\n");

	return NULL;
}

EXPORT_API void gub_ref()
{
	fprintf(stdout, "GUB: ref\n");
	if (gub_ref_count == 0) {
		GError *err = 0;

		if (!gst_init_check(0, 0, &err)) {
			fprintf(stderr, "GUB: Failed to initialize GStreamer: %s\n", err ? err->message : "<No error message>");
			return;
		}

		gub_main_loop_thread = g_thread_new("GstUnityBridge Main Thread\n", gub_main_loop_func, NULL);
		if (!gub_main_loop_thread) {
			fprintf(stderr, "GUB: Failed to create GLib main thread: %s\n", err ? err->message : "<No error message>");
			return;
		}

		fprintf(stdout, "GUB: GStreamer initialized\n");
	}

	gub_ref_count++;
}

EXPORT_API void gub_unref()
{
	fprintf(stdout, "GUB: unref\n");
	if (gub_ref_count == 0) {
		fprintf(stderr, "GUB: Trying to unref past zero\n");
		return;
	}

	gub_ref_count--;

	if (gub_ref_count == 0) {
		if (!gub_main_loop) {
			return;
		}
		g_main_loop_quit(gub_main_loop);
		gub_main_loop = NULL;
		g_thread_join(gub_main_loop_thread);
		gub_main_loop_thread = NULL;
	}
}

EXPORT_API gint32 gub_is_active()
{
	fprintf(stdout, "GUB: is_active\n");
	return (gub_ref_count > 0);
}

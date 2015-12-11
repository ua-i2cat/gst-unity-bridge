/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
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
	gub_log("Entering main loop");
	gub_main_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(gub_main_loop);
	gub_log("Quitting main loop");

	return NULL;
}

EXPORT_API void gub_ref()
{
	gub_log("GST ref");
	if (gub_ref_count == 0) {
		GError *err = 0;

		if (!gst_init_check(0, 0, &err)) {
			gub_log("Failed to initialize GStreamer: %s", err ? err->message : "<No error message>");
			return;
		}

		gub_main_loop_thread = g_thread_new("GstUnityBridge Main Thread", gub_main_loop_func, NULL);
		if (!gub_main_loop_thread) {
			gub_log("Failed to create GLib main thread: %s", err ? err->message : "<No error message>");
			return;
		}

		gub_log("GStreamer initialized");
	}

	gub_ref_count++;
}

EXPORT_API void gub_unref()
{
	gub_log("GST unref");
	if (gub_ref_count == 0) {
		gub_log("Trying to unref past zero");
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
	gub_log("is_active");
	return (gub_ref_count > 0);
}

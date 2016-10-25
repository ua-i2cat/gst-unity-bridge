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
#include "gub_gstreamer.h"

static GstClockTime _priv_gst_info_start_time;

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

static void gst_debug_gub(GstDebugCategory * category, GstDebugLevel level,
    const gchar * file, const gchar * function, gint line,
    GObject * object, GstDebugMessage * message, gpointer unused)
{
    GstClockTime elapsed;
    gchar *tag;
    const gchar *level_str;

    if (level > gst_debug_category_get_threshold(category))
        return;

    elapsed = GST_CLOCK_DIFF(_priv_gst_info_start_time,
        gst_util_get_timestamp());

    switch (level) {
    case GST_LEVEL_ERROR:
        level_str = "ERR";
        break;
    case GST_LEVEL_WARNING:
        level_str = "WRN";
        break;
    case GST_LEVEL_INFO:
        level_str = "NFO";
        break;
    case GST_LEVEL_DEBUG:
        level_str = "DBG";
        break;
    default:
        level_str = "LOG";
        break;
    }

    tag = g_strdup_printf("%s", gst_debug_category_get_name(category));

    if (object) {
        gchar *obj;

        if (GST_IS_PAD(object) && GST_OBJECT_NAME(object)) {
            obj = g_strdup_printf("<%s:%s>", GST_DEBUG_PAD_NAME(object));
        }
        else if (GST_IS_OBJECT(object) && GST_OBJECT_NAME(object)) {
            obj = g_strdup_printf("<%s>", GST_OBJECT_NAME(object));
        }
        else if (G_IS_OBJECT(object)) {
            obj = g_strdup_printf("<%s@%p>", G_OBJECT_TYPE_NAME(object), object);
        }
        else {
            obj = g_strdup_printf("<%p>", object);
        }

        gub_log(
            "%" GST_TIME_FORMAT " %p %s %s %s:%d:%s:%s %s",
            GST_TIME_ARGS(elapsed), g_thread_self(), level_str, tag,
            file, line, function, obj, gst_debug_message_get(message));

        g_free(obj);
    }
    else {
        gub_log(
            "%" GST_TIME_FORMAT " %p %s %s %s:%d:%s %s",
            GST_TIME_ARGS(elapsed), g_thread_self(), level_str, tag,
            file, line, function, gst_debug_message_get(message));
    }
    g_free(tag);
}

EXPORT_API void gub_ref(const char *gst_debug_string)
{
    gub_log("GST ref (%d -> %d)", gub_ref_count, gub_ref_count + 1);
    if (gub_ref_count == 0) {
        GError *err = 0;
        gchar *version = NULL;

        if (!gst_init_check(0, 0, &err)) {
            gub_log("Failed to initialize GStreamer: %s", err ? err->message : "<No error message>");
            return;
        }

        /* get time we started for debugging messages */
        _priv_gst_info_start_time = gst_util_get_timestamp();

        gst_debug_remove_log_function(gst_debug_log_default);
        gst_debug_add_log_function((GstLogFunction)gst_debug_gub, NULL, NULL);
        if (gst_debug_string) {
            gub_log("Setting debug level to %s", gst_debug_string);
            gst_debug_set_active(TRUE);
            gst_debug_set_threshold_from_string(gst_debug_string, TRUE);
        } else {
            gst_debug_set_active(FALSE);
        }

#if defined (__ANDROID__)
        gst_android_register_static_plugins();
        gst_android_load_gio_modules();
#endif

        gub_main_loop_thread = g_thread_new("GstUnityBridge Main Thread", gub_main_loop_func, NULL);
        if (!gub_main_loop_thread) {
            gub_log("Failed to create GLib main thread: %s", err ? err->message : "<No error message>");
            return;
        }

        version = gst_version_string();
        gub_log("%s initialized", version);
        g_free(version);
    }

    gub_ref_count++;
}

EXPORT_API void gub_unref()
{
    gub_log("GST unref (%d -> %d)", gub_ref_count, gub_ref_count - 1);
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

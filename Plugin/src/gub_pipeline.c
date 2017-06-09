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

#include <gst/video/video.h>
#include <gst/net/gstnet.h>
#include <gst/pbutils/encoding-profile.h>
#include <gstdvbcsswcclient.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include "gub.h"
#include "gub_pipeline.h"

#define MAX_JITTERBUFFER_DELAY_MS 40
#define MAX_PIPELINE_DELAY_MS 500

struct _GUBPipeline {
    char *name;
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
    int video_width, video_height;

    GUBPipelineOnEosPFN on_eos_handler;
    GUBPipelineOnErrorPFN on_error_handler;
    GUBPipelineOnQosPFN on_qos_handler;
    void *userdata;

    GstAppSrc *appsrc;
    GstClockTime basetime;
    gboolean synced;
};

void gub_log_pipeline(GUBPipeline *pipeline, const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    gchar *new_format = g_strdup_printf("[%s] %s", pipeline->name, format);
    gchar *final_string = g_strdup_vprintf(new_format, argptr);
    g_free(new_format);
    va_end(argptr);
    gub_log("%s", final_string);
    g_free(final_string);
}

EXPORT_API void *gub_pipeline_create(const char *name,
    GUBPipelineOnEosPFN eos_handler, GUBPipelineOnErrorPFN error_handler, GUBPipelineOnQosPFN qos_handler,
    void *userdata)
{
    GUBPipeline *pipeline = (GUBPipeline *)malloc(sizeof(GUBPipeline));
    memset(pipeline, 0, sizeof(GUBPipeline));
    pipeline->name = g_strdup(name);
    pipeline->on_eos_handler = eos_handler;
    pipeline->on_error_handler = error_handler;
    pipeline->on_qos_handler = qos_handler;
    pipeline->userdata = userdata;

    return pipeline;
}

EXPORT_API void gub_pipeline_close(GUBPipeline *pipeline)
{
    gub_destroy_graphic_context(pipeline->graphic_context);
    if (pipeline->pipeline) {
        gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_NULL);
        gst_object_unref(pipeline->pipeline);
    }
    if (pipeline->last_sample) {
        gst_sample_unref(pipeline->last_sample);
    }
    memset(pipeline, 0, sizeof(GUBPipeline));
}

EXPORT_API void gub_pipeline_destroy(GUBPipeline *pipeline)
{
    gub_pipeline_close(pipeline);
    g_free(pipeline->name);
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

EXPORT_API double gub_pipeline_get_duration(GUBPipeline *pipeline)
{
    gint64 duration = GST_CLOCK_TIME_NONE;
    gst_element_query_duration(pipeline->pipeline, GST_FORMAT_TIME, &duration);
    return duration / (double)GST_SECOND;
}

EXPORT_API double gub_pipeline_get_position(GUBPipeline *pipeline)
{
    gint64 position = GST_CLOCK_TIME_NONE;
    gst_element_query_position(pipeline->pipeline, GST_FORMAT_TIME, &position);
    return position / (double)GST_SECOND;
}

EXPORT_API void gub_pipeline_set_position(GUBPipeline *pipeline, double position)
{
    gst_element_seek_simple(pipeline->pipeline,
        GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT,
        (gint64)(position * GST_SECOND));
}

EXPORT_API void gub_pipeline_set_basetime(GUBPipeline *pipeline, guint64 basetime)
{
    gst_element_set_base_time(pipeline->pipeline, (GstClockTime)basetime);
}

static gboolean select_stream(GstBin *rtspsrc, guint num, GstCaps *caps, GUBPipeline *pipeline)
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

    g_signal_connect(source, "select-stream", G_CALLBACK(select_stream), pipeline);
}

static void sync_video_position(GUBPipeline *pipeline) 
{    
    if (!pipeline->synced) {
        gint64 position = GST_CLOCK_TIME_NONE;
	gint64 current_time = gst_clock_get_time(pipeline->net_clock) + MAX_PIPELINE_DELAY_MS*GST_MSECOND;
	if (current_time < pipeline->basetime) {
	    gub_log_pipeline(pipeline, "ERROR: %lldns : %lldns", current_time, pipeline->basetime);
	} else {
	    gboolean seek = gst_element_seek_simple(pipeline->pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, current_time - pipeline->basetime);
	    gub_log_pipeline(pipeline, "Seeked: %lldns : %d", current_time - pipeline->basetime, seek);	    
	    while(position == GST_CLOCK_TIME_NONE){gst_element_query_position(pipeline->pipeline, GST_FORMAT_TIME, &position);}
	    gub_log_pipeline(pipeline, "Setting basetime to %lldns + position %lldns", pipeline->basetime, position);
	}
	gst_element_set_base_time(pipeline->pipeline, pipeline->basetime+position); //pipeline->basetime+position);

	// Disable GstPipeline automatic handling of basetime
	gst_element_set_start_time(pipeline->pipeline, GST_CLOCK_TIME_NONE);
	pipeline->synced = TRUE;
    }
}

static void message_received(GstBus *bus, GstMessage *message, GUBPipeline *pipeline) {
    switch (GST_MESSAGE_TYPE(message)) {
	case GST_MESSAGE_ERROR:
	{
	    GError *error = NULL;
	    gchar *debug = NULL;
	    gchar *full_msg = NULL;

	    gst_message_parse_error(message, &error, &debug);
	    full_msg = g_strdup_printf("[%s] %s (%s)", pipeline->name, error->message, debug);
	    gub_log_error(full_msg);
	    if (pipeline->on_error_handler != NULL) {
		pipeline->on_error_handler(pipeline->userdata, error->message);
	    }
	    g_error_free(error);
	    g_free(debug);
	    g_free(full_msg);
	    break;
	}
	case GST_MESSAGE_EOS:
	    if (pipeline->on_eos_handler != NULL) {
		pipeline->on_eos_handler(pipeline->userdata);
	    }
	    break;
	case GST_MESSAGE_QOS:
	    if (pipeline->on_qos_handler != NULL) {
		guint64 running_time;
		guint64 stream_time;
		guint64 timestamp;
		gint64 jitter;
		gdouble proportion;
		guint64 processed;
		guint64 dropped;
		gst_message_parse_qos(message, NULL, &running_time, &stream_time, &timestamp, NULL);
		gst_message_parse_qos_values(message, &jitter, &proportion, NULL);
		gst_message_parse_qos_stats(message, NULL, &processed, &dropped);
		pipeline->on_qos_handler(pipeline->userdata, jitter, running_time, stream_time, timestamp, proportion, processed, dropped);
	    }
	    break;
	case GST_MESSAGE_STATE_CHANGED:
	    if (GST_MESSAGE_SRC(message) == GST_OBJECT(pipeline->pipeline)) {
		GstState old_state, new_state, pending_state;
		gst_message_parse_state_changed(message, &old_state, &new_state, &pending_state);
		g_print("\nPipeline state changed from %s to %s:\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
		if (new_state == GST_STATE_PAUSED) {
		    sync_video_position(pipeline);
		}
	    }
	    break;
    }
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

EXPORT_API void gub_pipeline_setup_decoding_clock(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index,
    const gchar *net_clock_addr, int net_clock_port, guint64 basetime,
    float crop_left, float crop_top, float crop_right, float crop_bottom, gboolean isDvbWc)
{
    GError *err = NULL;
    GstElement *vsink;
    gchar *full_pipeline_description = NULL;
    GstBus *bus = NULL;

    if (pipeline->pipeline) {
        gub_pipeline_close(pipeline);
    }

    full_pipeline_description = g_strdup_printf("playbin3 uri=%s", uri);
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

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline->pipeline));
    gst_bus_add_signal_watch(bus);
    gst_object_unref(bus);
    g_signal_connect(bus, "message", G_CALLBACK(message_received), pipeline);

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
            gst_object_unref(sink);
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
	    if (isDvbWc == FALSE) {
		pipeline->net_clock = gst_net_client_clock_new("net_clock", net_clock_addr, net_clock_port, 0);
	    }
	    else{
		pipeline->net_clock = gst_dvb_css_wc_client_clock_new("dvb_clock", net_clock_addr, net_clock_port, 0);
	    }
        if (!pipeline->net_clock) {
            gub_log_pipeline(pipeline, "Could not create network clock at %s %d", net_clock_addr, net_clock_port);
            return;
        }

	gint64 current_time = gst_clock_get_time(pipeline->net_clock);
	gub_log_pipeline(pipeline, "CURRENT_TIME: %lldns", current_time);
	current_time = gst_clock_get_internal_time(pipeline->net_clock);
	gub_log_pipeline(pipeline, "INTERNAL_TIME: %lldns", current_time);
	
        start = g_get_monotonic_time();
        gst_clock_wait_for_sync(pipeline->net_clock, 30 * GST_SECOND);
        stop = g_get_monotonic_time();
        if (gst_clock_is_synced(pipeline->net_clock)) {
            gub_log_pipeline(pipeline, "Synchronized to network clock in %g seconds", (stop - start) / 1e6);
        }
        else {
            gub_log_pipeline(pipeline, "Could not synchronize to network clock after %g seconds", (stop - start) / 1e6);
        }
	
	current_time = gst_clock_get_time(pipeline->net_clock);
	gub_log_pipeline(pipeline, "CURRENT_TIME2: %lldns", current_time);
	current_time = gst_clock_get_internal_time(pipeline->net_clock);
	gub_log_pipeline(pipeline, "INTERNAL_TIME2: %lldns", current_time);

        gst_pipeline_use_clock(GST_PIPELINE(pipeline->pipeline), pipeline->net_clock);
        gst_pipeline_set_latency(GST_PIPELINE(pipeline->pipeline), MAX_PIPELINE_DELAY_MS * GST_MSECOND);
    }
    
    pipeline->basetime = basetime;
    pipeline->synced = (basetime == 0);
}

EXPORT_API void gub_pipeline_setup_decoding(GUBPipeline *pipeline, const gchar *uri, int video_index, int audio_index,
	const gchar *net_clock_addr, int net_clock_port, guint64 basetime,
	float crop_left, float crop_top, float crop_right, float crop_bottom)
{
	gub_pipeline_setup_decoding_clock(pipeline, uri, video_index, audio_index, net_clock_addr, net_clock_port, basetime, crop_left, crop_top, crop_right, crop_bottom, FALSE);
}

EXPORT_API gint32 gub_pipeline_grab_frame(GUBPipeline *pipeline, int *width, int *height)
{
    //GST_DEBUG_BIN_TO_DOT_FILE((GstBin*)pipeline->pipeline, GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");
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
     //   gub_log_pipeline(pipeline, "Pipeline does not contain a sink named 'sink'");
        return 0;
    }

    g_object_get(sink, "last-sample", &pipeline->last_sample, NULL);
    gst_object_unref(sink);
    if (!pipeline->last_sample) {
      //  gub_log_pipeline(pipeline, "Could not read property 'last-sample' from sink %s",
        //    gst_plugin_feature_get_name(gst_element_get_factory(sink)));
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
        *width = (int)(info.width  * (1 - pipeline->video_crop_left - pipeline->video_crop_right));
        *height = (int)(info.height * (1 - pipeline->video_crop_top - pipeline->video_crop_bottom));
    }
    else {
        *width = info.width;
        *height = info.height;
    }

    return 1;
}

EXPORT_API void gub_pipeline_blit_image(GUBPipeline *pipeline, void *_TextureNativePtr)
{
	if (!pipeline)
	{
		return;
	}

    if (!pipeline->last_sample) {
        return;
    }

    gub_blit_image(pipeline->graphic_context, pipeline->last_sample, _TextureNativePtr);

    gst_sample_unref(pipeline->last_sample);
    pipeline->last_sample = NULL;
}

GstEncodingProfile * gub_pipeline_create_mp4_h264_profile(void)
{
    GstEncodingContainerProfile *prof;
    GstCaps *caps;

    caps = gst_caps_from_string("video/quicktime,variant=iso");
    prof = gst_encoding_container_profile_new("mp4/H.264", "Standard mp4/H.264", caps, NULL);
    gst_caps_unref(caps);

    caps = gst_caps_from_string("video/x-h264,profile=main");
    gst_encoding_container_profile_add_profile(prof, (GstEncodingProfile*)gst_encoding_video_profile_new(caps, NULL, NULL, 0));
    gst_caps_unref(caps);

    return (GstEncodingProfile*)prof;
}

EXPORT_API void gub_pipeline_setup_encoding(GUBPipeline *pipeline, const gchar *filename,
    int width, int height)
{
    GError *err = NULL;
    GstElement *encodebin, *filesink;
    GstCaps *raw_caps;
    gchar *raw_caps_description = NULL;
    GstBus *bus = NULL;
    GstPad *encodebin_sink_pad = NULL, *appsrc_src_pad = NULL;

    if (pipeline->pipeline) {
        gub_pipeline_close(pipeline);
    }

    pipeline->pipeline = gst_pipeline_new(NULL);
    pipeline->video_width = width;
    pipeline->video_height = height;

    raw_caps_description = g_strdup_printf("video/x-raw,format=RGBA,width=%d,height=%d", width, height);
    raw_caps = gst_caps_from_string(raw_caps_description);
    gub_log_pipeline(pipeline, "Using video caps: %s", raw_caps_description);
    g_free(raw_caps_description);

    pipeline->appsrc = GST_APP_SRC(gst_element_factory_make("appsrc", "source"));
    gub_log_pipeline(pipeline, "Using appsrc: %p", pipeline->appsrc);
    gst_app_src_set_caps(pipeline->appsrc, raw_caps);
    g_object_set(pipeline->appsrc,
        "stream-type", 0,
        "max-bytes", width*height * 4 * 10,
        "is-live", TRUE,
        "do-timestamp", TRUE,
        "format", GST_FORMAT_TIME,
        "min-latency", 0, NULL);

    encodebin = gst_element_factory_make("encodebin", NULL);
    gub_log_pipeline(pipeline, "Using encodebin: %p", encodebin);
    g_object_set(encodebin, "profile", gub_pipeline_create_mp4_h264_profile(), NULL);
    g_signal_emit_by_name(encodebin, "request-pad", raw_caps, &encodebin_sink_pad);
    gub_log_pipeline(pipeline, "Using encodebin_sink_pad: %p", encodebin_sink_pad);

    filesink = gst_element_factory_make("filesink", NULL);
    g_object_set(filesink, "location", filename, NULL);
    gub_log_pipeline(pipeline, "Using filesink: %p, location: %s", filesink, filename);

    gst_bin_add_many(GST_BIN(pipeline->pipeline), GST_ELEMENT(pipeline->appsrc), encodebin, filesink, NULL);

    appsrc_src_pad = gst_element_get_static_pad(GST_ELEMENT(pipeline->appsrc), "src");
    gst_pad_link(appsrc_src_pad, encodebin_sink_pad);
    gst_element_link(encodebin, filesink);

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline->pipeline));
    gst_bus_add_signal_watch(bus);
    gst_object_unref(bus);
    g_signal_connect(bus, "message", G_CALLBACK(message_received), pipeline);
}

EXPORT_API void gub_pipeline_consume_image(GUBPipeline *pipeline, guint8 *rawdata, int size)
{
    GstBuffer *buffer = gst_buffer_new_allocate(NULL, size, NULL);
    GstMapInfo mi;
    int line;
    int stride = pipeline->video_width * 4;

    gst_buffer_map(buffer, &mi, GST_MAP_WRITE);
    for (line = 0; line < pipeline->video_height; line++)
    {
        memcpy(mi.data + line * stride, rawdata + (pipeline->video_height - 1 - line) * stride, stride);
    }
    gst_buffer_unmap(buffer, &mi);

    if (pipeline->playing == FALSE && pipeline->play_requested == TRUE) {
        gub_log_pipeline(pipeline, "Setting pipeline to PLAYING");
        gst_element_set_state(GST_ELEMENT(pipeline->pipeline), GST_STATE_PLAYING);
        gub_log_pipeline(pipeline, "State change completed");
        pipeline->playing = TRUE;
    }

    gst_app_src_push_buffer(pipeline->appsrc, buffer);
}

EXPORT_API void gub_pipeline_stop_encoding(GUBPipeline *pipeline)
{
    if (pipeline && pipeline->appsrc != NULL) {
        gst_app_src_end_of_stream(pipeline->appsrc);
    }
}

EXPORT_API void gub_pipeline_set_volume(GUBPipeline *pipeline, gdouble volume)
{
    if (volume < 0.f) volume = 0.f;
    if (volume > 1.f) volume = 1.f;

    gub_log_pipeline(pipeline, "Setting volume to %g", volume);
    g_object_set(pipeline->pipeline, "volume", volume, NULL);
}

static gint compare_name_func(const GValue *value, const gchar *pattern)
{
    GstElement *element = g_value_get_object(value);
    return g_regex_match_simple (pattern, g_type_name(gst_element_factory_get_element_type(gst_element_get_factory(element))), 0, 0) ? 0 : 1;
}

EXPORT_API void gub_pipeline_set_adaptive_bitrate_limit(GUBPipeline *pipeline, gfloat bitrate_limit)
{
    GValue elem = G_VALUE_INIT;
    GstIterator *it = gst_bin_iterate_recurse(GST_BIN(pipeline->pipeline));
    if (gst_iterator_find_custom(it, (GCompareFunc)compare_name_func, &elem, "^(GstDashDemux|GstHLSDemux)$"))
    {
        GstElement *demux = GST_ELEMENT(g_value_get_object(&elem));
        gchar *name = gst_element_get_name(demux);
        gub_log_pipeline(pipeline, "Setting bitrate-limit of adaptive demuxer %s to %f", name, bitrate_limit);
        g_free(name);
        g_object_set(demux, "bitrate-limit", bitrate_limit, NULL);
    }
    else
    {
        gub_log_pipeline(pipeline, "Could not find any adaptive demuxer in the pipeline (Have you waited for OnStart before setting the bitrate?)");
    }
    gst_iterator_free(it);
}

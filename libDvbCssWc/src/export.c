/* GStreamer
 * Copyright (C) 2016 Tomasz Merda <merda@man.poznan.pl>
 * Copyright (C) 2016 Wojciech Kapsa <kapsa@man.poznan.pl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <stdlib.h>
#include "gstdvbcsswcserver.h"
#include <gst/net/gstnet.h>
#include "export.h"
#include <time.h>

typedef struct _Server {
	void *gstdvbcsswcserver;
	GMainLoop *loop;
	GstClock *clock;
	GThread *thread;
} Server;

static Server* sServer = NULL;
G_LOCK_DEFINE_STATIC(mutex);

extern EXPORT_API Server* server_new(void){
	Server* s = NULL;
	s = malloc(sizeof(Server));
	s->gstdvbcsswcserver = NULL;
	s->loop = NULL;
	s->clock = NULL;
	s->thread = NULL;
	return s;
}

void server_free(Server** s) {
	if((*s)->loop){
		gst_object_unref((*s)->loop);
		(*s)->loop = NULL;
	}
	if((*s)->thread){
		gst_object_unref((*s)->thread);
		(*s)->thread = NULL;
	}
	if((*s)->clock){
		gst_object_unref((*s)->clock);
		(*s)->clock = NULL;
	}
	if((*s)->gstdvbcsswcserver){
		gst_object_unref((*s)->gstdvbcsswcserver);
		(*s)->gstdvbcsswcserver = NULL;
	}

	free(*s);
	*s = NULL;
}

extern EXPORT_API gboolean gst_dvb_css_wc_start(const gchar *address, gint port, gboolean followup, guint32 max_freq_error_ppm,  gboolean isDebug) {
	g_print("dvb_css_wc_start\n");
	G_LOCK(mutex);
	sServer = server_new();
	if (sServer == NULL) {
		goto server_struct_not_initialized;
	}
	gst_init(NULL, NULL);
	sServer->loop = g_main_loop_new(NULL, FALSE);
	sServer->clock = gst_system_clock_obtain();

	if(isDebug == FALSE){
		sServer->gstdvbcsswcserver = gst_dvb_css_wc_server_new(sServer->clock, address, port, followup, max_freq_error_ppm);		
	}
	else{
		sServer->gstdvbcsswcserver = gst_net_time_provider_new(sServer->clock, address, port);		
	}
	
	if (sServer->gstdvbcsswcserver == NULL) {
		g_print("gstdvbcsswcserver not created\n");
		goto cleanup;
	}

	g_object_get(sServer->gstdvbcsswcserver, "port", &port, NULL);
	g_print("Published network clock on port %u\n", port);

	sServer->thread = g_thread_try_new("dvb_css_wc_thread", (GThreadFunc) g_main_loop_run, sServer->loop, NULL);
	if (sServer->thread == NULL) {
		g_print("Thread not created\n");
		goto cleanup;
	}
	
	g_print("Server started\n");	
	G_UNLOCK(mutex);
	return TRUE;

	/* ERRORS */
server_struct_not_initialized:{
	g_print("Server struct not initialized\n");
	G_UNLOCK(mutex);
	return FALSE;
	}
cleanup:{
	server_free(&sServer);
	G_UNLOCK(mutex);
	return FALSE;
	}
}

extern EXPORT_API void gst_dvb_css_wc_stop(void) {
	g_print("dvb_css_wc_stop\n");
	G_LOCK(mutex);
	if (sServer == NULL) {
		G_UNLOCK(mutex);
		return;
	}
	g_print("Stopping server\n");
	if (sServer->loop != NULL) {
		g_main_loop_quit(sServer->loop);
		g_thread_join(sServer->thread);
	}
	server_free(&sServer);
	g_print("Server stopped\n");
	G_UNLOCK(mutex);
}

extern EXPORT_API GstClockTime gst_dvb_css_wc_get_time(void) {
	GstClockTime time;
	g_print("dvb_css_wc_get_time\n");
	G_LOCK(mutex);
	if (sServer != NULL && sServer->clock != NULL) {
		time = gst_clock_get_time(sServer->clock);
		G_UNLOCK(mutex);
		return time;
	}
	else {
		g_print("Server not ready\n");
	}
	G_UNLOCK(mutex);
	return 0;
}

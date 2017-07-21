/* GStreamer
 * Copyright (C) 2016 Daniel Piesik <dpiesik@man.poznan.pl>
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

#include "../gstdvbcsswcclient.h"
#include "client-export.h"

extern EXPORT_API GstClock*
gst_dvb_css_wc_client_start(const gchar *name, const gchar *remote_address, gint remote_port, GstClockTime base_time)
{
	GstClock* dvb_css_wc_client = gst_dvb_css_wc_client_clock_new(name, remote_address, remote_port, base_time);
	if(dvb_css_wc_client == NULL)
	{
		GST_ERROR("dvb_css_wc_client clock not created\n");
		return NULL;
	}

	return dvb_css_wc_client;
}

extern EXPORT_API void
gst_dvb_css_wc_client_stop(GstClock *dvb_css_wc_client)
{
	if(dvb_css_wc_client != NULL)
	{
		gst_object_unref(dvb_css_wc_client);
		dvb_css_wc_client = NULL;
	}
}

extern EXPORT_API gboolean
gst_dvb_css_wc_client_is_synced(GstClock *dvb_css_wc_client)
{
	if(dvb_css_wc_client != NULL)
	{
		return gst_clock_is_synced(dvb_css_wc_client);
	}
	return FALSE;
}

extern EXPORT_API GstClockTime
gst_dvb_css_wc_client_get_time(GstClock *dvb_css_wc_client)
{
	if(dvb_css_wc_client != NULL)
	{
		return gst_clock_get_time(dvb_css_wc_client);
	}
	return 0;
}

extern EXPORT_API GstClockTime
gst_dvb_css_wc_client_get_time_local(GstClock *dvb_css_wc_client)
{
	GstClockTime time = 0;
	GstClock* clock = gst_system_clock_obtain();
	time = gst_clock_get_time(clock);
	gst_object_unref(clock);
	return time;
}
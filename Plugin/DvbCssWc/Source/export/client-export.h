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

#ifndef __DVB_CSS_WC_CLIENT_EXPORT_H__
#define __DVB_CSS_WC_CLIENT_EXPORT_H__
#include <gst/gst.h>

#ifdef _WIN32
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

/**
 * gst_dvb_css_wc_client_start:
 * @name: a name for the client
 * @remote_address: the address or hostname of the remote clock provider
 * @remote_port: the port of the remote clock provider
 * @base_time: initial time of the clock
 *
 * Create a new #GstClock that will report the time
 * provided by the wall clock server on @remote_address and @remote_port.
 *
 * Returns: a new #GstClock* that receives a time from the remote clock.
 */
extern EXPORT_API GstClock* gst_dvb_css_wc_client_start(const gchar *name, const gchar *remote_address, gint remote_port, GstClockTime base_time);

/**
 * gst_dvb_css_wc_stop:
 * @dvb_css_wc_client: client that will be stopped
 *
 * Stop reporting the time from the remote clock
 *
 * Returns: none
 */
extern EXPORT_API void gst_dvb_css_wc_client_stop(GstClock *dvb_css_wc_client);

/**
 * gst_dvb_css_wc_client_is_synced:
 * @dvb_css_wc_client: client to be checked if is synchronized with the remote clock
 *
 * Get information about synchronization with the remote clock
 *
 * Returns: TRUE if is synchronized or FALSE in other case
 */
extern EXPORT_API gboolean gst_dvb_css_wc_client_is_synced(GstClock *dvb_css_wc_client);

/**
 * gst_dvb_css_wc_client_get_time:
 * @dvb_css_wc_client: client that will return a time
 *
 * Get the current time from the client
 *
 * Returns: 0 on error or GstClockTime
 */
extern EXPORT_API GstClockTime gst_dvb_css_wc_client_get_time(GstClock *dvb_css_wc_client);

/**
* gst_dvb_css_wc_client_get_time_local:
* @dvb_css_wc_client: client that will return a time
*
* Get the system time
*
* Returns: 0 on error or GstClockTime
*/
extern EXPORT_API GstClockTime gst_dvb_css_wc_client_get_time_local(GstClock *dvb_css_wc_client);


#endif /* __DVB_CSS_WC_CLIENT_EXPORT_H__ */

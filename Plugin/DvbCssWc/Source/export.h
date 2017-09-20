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

#ifndef __EXPORT_H__
#define __EXPORT_H__
#include <gst/gst.h>

#ifdef _WIN32
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API
#endif

/**
 * gst_dvb_css_wc_start:
 * @address: (allow-none): an address to bind on as a dotted quad
 *           (xxx.xxx.xxx.xxx), IPv6 address, or NULL to bind to all addresses
 * @port: a port to bind on, or 0 to let the kernel choose
 * @max_freq_error_ppm The clock maximum frequency error in parts-per-million
 * @followup Set to True if the Wall Clock Server should send follow-up responses
 *
 * Creates and starts DVC CSS WC clock
 *
 */
extern EXPORT_API gboolean gst_dvb_css_wc_start(const gchar *address, gint port, gboolean followup, guint32 max_freq_error_ppm, gboolean isDebug);

/**
 * gst_dvb_css_wc_stop:
 * Stops DVC CSS WC clock
 */
extern EXPORT_API void gst_dvb_css_wc_stop(void);

/**
 * gst_dvb_css_wc_get_time:
 * Get current DVC CSS WC clock time
 * Returns: 0 on error or GstClockTime
 */
extern EXPORT_API GstClockTime gst_dvb_css_wc_get_time(void);

#endif

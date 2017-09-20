/* GStreamer
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


#ifndef __GST_DVB_CSS_WC_PACKET_H__
#define __GST_DVB_CSS_WC_PACKET_H__

#include <gst/gst.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * GST_DVB_CSS_WC_PACKET_SIZE:
 *
 * The size of the packets sent between DVB-CSS-WC clocks.
 */
#define GST_DVB_CSS_WC_PACKET_SIZE 32
#define GST_DVB_CSS_WC_VERSION 0

#define GST_DVB_CSS_WC_TYPE_PACKET gst_dvb_css_wc_packet_get_type ()
typedef struct _GstDvbCssWcPacket GstDvbCssWcPacket;

enum {
    GST_DVB_CSS_WC_MSG_REQUEST = 0,
    GST_DVB_CSS_WC_MSG_RESPONSE = 1,
    GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP = 2,
    GST_DVB_CSS_WC_MSG_FOLLOWUP = 3
};

/**
 * GstDvbCssWcPacket 
 */
struct _GstDvbCssWcPacket {
    guint8 version;
    guint8 message_type;
    gint8 precision;
    gint8 reserved;
    guint32 max_freq_error;

    guint32 originate_timevalue_secs;
    guint32 originate_timevalue_nanos;
    GstClockTime receive_timevalue;
    GstClockTime transmit_timevalue;

    // this is not part of packet to send
    GstClockTime response_timevalue;
};

GType gst_dvb_css_wc_packet_get_type(void);

GstDvbCssWcPacket* gst_dvb_css_wc_packet_new(const guint8 *buffer);
GstDvbCssWcPacket* gst_dvb_css_wc_packet_copy(const GstDvbCssWcPacket *packet);
void gst_dvb_css_wc_packet_free(GstDvbCssWcPacket *packet);

guint8* gst_dvb_css_wc_packet_serialize(const GstDvbCssWcPacket *packet);

GstDvbCssWcPacket* gst_dvb_css_wc_packet_receive(GSocket * socket,
        GSocketAddress ** src_address,
        GError ** error);

gboolean gst_dvb_css_wc_packet_send(const GstDvbCssWcPacket * packet,
        GSocket * socket,
        GSocketAddress * dest_address,
        GError ** error);

gint8 gst_dvb_css_wc_packet_encode_precision(gdouble precisionSecs);
gdouble gst_dvb_css_wc_packet_decode_precision(gint8 precision);
guint32 gst_dvb_css_wc_packet_encode_max_freq_error(gdouble max_freq_error_ppm);
gdouble gst_dvb_css_wc_packet_decode_max_freq_error(guint32 max_freq_error);

GstClockTime wc_timestamp_to_gst_clock_time (guint32 seconds, guint32 fraction);
guint32 gst_clock_time_to_wc_timestamp_seconds (GstClockTime gst);
guint32 gst_clock_time_to_wc_timestamp_fraction (GstClockTime gst);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstDvbCssWcPacket, gst_dvb_css_wc_packet_free)
#endif

G_END_DECLS

#endif /* __GST_DVB_CSS_WC_PACKET_H__ */

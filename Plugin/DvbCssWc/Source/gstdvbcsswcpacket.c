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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <math.h>

#ifdef __CYGWIN__
# include <unistd.h>
# include <fcntl.h>
#endif

#include "gstdvbcsswcpacket.h"

GST_DEBUG_CATEGORY_STATIC (dvbcss_wc_packet);
#define GST_CAT_DEFAULT   (dvbcss_wc_packet)
#define GST_GET_LEVEL (gst_debug_category_get_threshold(dvbcss_wc_packet))
#define _do_init GST_DEBUG_CATEGORY_INIT (dvbcss_wc_packet, "dvbcss_wc_packet", 0, "DVB CSS WC packets");

G_DEFINE_BOXED_TYPE_WITH_CODE (GstDvbCssWcPacket, gst_dvb_css_wc_packet,
    gst_dvb_css_wc_packet_copy, gst_dvb_css_wc_packet_free, _do_init);

GstClockTime
wc_timestamp_to_gst_clock_time (guint32 seconds, guint32 fraction)
{
  return gst_util_uint64_scale(seconds, GST_SECOND, 1) + fraction;
}

guint32
gst_clock_time_to_wc_timestamp_seconds (GstClockTime gst)
{
  GstClockTime seconds = gst_util_uint64_scale (gst, 1, GST_SECOND);
  return (guint32)seconds;
}

guint32
gst_clock_time_to_wc_timestamp_fraction (GstClockTime gst)
{
  GstClockTime seconds = gst_util_uint64_scale (gst, 1, GST_SECOND);    
  return (guint32)(gst - (seconds * GST_SECOND));
}

#define PACKET_LOG(info, packet) \
    if(packet != NULL && GST_GET_LEVEL >= GST_LEVEL_LOG){ \
    GST_LOG("%s:\n"                 \
            "version: %u\n"         \
            "message_type: %u\n"    \
            "precision: %i\n"       \
            "reserved: %u\n"        \
            "max_freq_error: %u\n"  \
            "originate_timevalue: %" GST_TIME_FORMAT "\n" \
            "receive_timevalue: %" GST_TIME_FORMAT "\n"   \
            "transmit_timevalue: %" GST_TIME_FORMAT "\n"  \
            "response_timevalue: %" GST_TIME_FORMAT,      \
            info,                   \
            packet->version,        \
            packet->message_type,   \
            packet->precision,      \
            packet->reserved,       \
            packet->max_freq_error, \
            GST_TIME_ARGS(wc_timestamp_to_gst_clock_time(packet->originate_timevalue_secs, packet->originate_timevalue_nanos)), \
            GST_TIME_ARGS(packet->receive_timevalue),   \
            GST_TIME_ARGS(packet->transmit_timevalue),  \
            GST_TIME_ARGS(packet->response_timevalue)); \
    }

/**
 * gst_dvb_css_wc_packet_new:
 * @buffer: (array): a buffer from which to construct the packet, or NULL
 *
 * Creates a new #GstDvbCssWcPacket from a buffer received over the network. The
 * caller is responsible for ensuring that @buffer is at least
 * #GST_DVB_CSS_WC_PACKET_SIZE bytes long.
 *
 * If @buffer is #NULL, the local and remote times will be set to
 * #GST_CLOCK_TIME_NONE.
 *
 * MT safe. Caller owns return value (gst_dvb_css_wc_packet_free to free).
 *
 * Returns: The new #GstDvbCssWcPacket.
 */
GstDvbCssWcPacket *
gst_dvb_css_wc_packet_new (const guint8 * buffer)
{
  GstDvbCssWcPacket *ret;
  GType init = GST_DVB_CSS_WC_TYPE_PACKET;

  g_assert (sizeof (GstClockTime) == 8);

  ret = g_new0 (GstDvbCssWcPacket, 1);

  if (buffer) {
      ret->version = GST_READ_UINT8(buffer);
      ret->message_type = GST_READ_UINT8(buffer + 1);
      ret->precision = GST_READ_UINT8(buffer + 2);
      ret->reserved = GST_READ_UINT8(buffer + 3);
      ret->max_freq_error = GST_READ_UINT32_BE(buffer + 4);
      
      ret->originate_timevalue_secs = GST_READ_UINT32_BE(buffer + 8);
      ret->originate_timevalue_nanos = GST_READ_UINT32_BE(buffer + 12);
      ret->receive_timevalue = wc_timestamp_to_gst_clock_time(GST_READ_UINT32_BE(buffer + 16), GST_READ_UINT32_BE(buffer + 20));
      ret->transmit_timevalue = wc_timestamp_to_gst_clock_time(GST_READ_UINT32_BE(buffer + 24), GST_READ_UINT32_BE(buffer + 28));
      ret->response_timevalue = 0;
  } else {
      ret->version = GST_DVB_CSS_WC_VERSION;
      ret->message_type = GST_DVB_CSS_WC_MSG_REQUEST;
      ret->precision = 0;
      ret->reserved = 0;
      ret->max_freq_error = 0;
      
      ret->originate_timevalue_secs = 0;
      ret->originate_timevalue_nanos = 0;
      ret->receive_timevalue = 0;
      ret->transmit_timevalue = 0;
      ret->response_timevalue = 0;
  }

  return ret;
}

/**
 * gst_dvb_css_wc_packet_free:
 * @packet: the #GstDvbCssWcPacket
 *
 * Free @packet.
 */
void
gst_dvb_css_wc_packet_free (GstDvbCssWcPacket * packet)
{
  g_free (packet);
}

/**
 * gst_dvb_css_wc_packet_copy:
 * @packet: the #GstDvbCssWcPacket
 *
 * Make a copy of @packet.
 *
 * Returns: a copy of @packet, free with gst_dvb_css_wc_packet_free().
 */
GstDvbCssWcPacket *
gst_dvb_css_wc_packet_copy (const GstDvbCssWcPacket * packet)
{
  GstDvbCssWcPacket *ret;

  ret = g_new0 (GstDvbCssWcPacket, 1);
  ret->version = packet->version;
  ret->message_type = packet->message_type;
  ret->precision = packet->precision;
  ret->reserved = packet->reserved;
  ret->max_freq_error = packet->max_freq_error;
  ret->originate_timevalue_secs = packet->originate_timevalue_secs;
  ret->originate_timevalue_nanos = packet->originate_timevalue_nanos;
  ret->receive_timevalue = packet->receive_timevalue;
  ret->transmit_timevalue = packet->transmit_timevalue;
  ret->response_timevalue = packet->response_timevalue;

  return ret;
}

/**
 * gst_dvb_css_wc_packet_serialize:
 * @packet: the #GstDvbCssWcPacket
 *
 * Serialized a #GstDvbCssWcPacket into a newly-allocated sequence of
 * #GST_DVB_CSS_WC_PACKET_SIZE bytes, in network byte order. The value returned is
 * suitable for passing to write(2) or sendto(2) for communication over the
 * network.
 *
 * MT safe. Caller owns return value (g_free to free).
 *
 * Returns: A newly allocated sequence of #GST_DVB_CSS_WC_PACKET_SIZE bytes.
 */
guint8 *
gst_dvb_css_wc_packet_serialize (const GstDvbCssWcPacket * packet)
{
  guint8 *ret;

  g_assert (sizeof (GstClockTime) == 8);

  ret = g_new0 (guint8, GST_DVB_CSS_WC_PACKET_SIZE);

  GST_WRITE_UINT8(ret, packet->version);  
  GST_WRITE_UINT8(ret + 1, packet->message_type);
  GST_WRITE_UINT8(ret + 2, packet->precision);
  GST_WRITE_UINT8(ret + 3, packet->reserved);
  GST_WRITE_UINT32_BE(ret + 4, packet->max_freq_error);
  
  GST_WRITE_UINT32_BE(ret + 8, packet->originate_timevalue_secs);
  GST_WRITE_UINT32_BE(ret + 12, packet->originate_timevalue_nanos);
  
  GST_WRITE_UINT32_BE(ret + 16, gst_clock_time_to_wc_timestamp_seconds(packet->receive_timevalue));
  GST_WRITE_UINT32_BE(ret + 20, gst_clock_time_to_wc_timestamp_fraction(packet->receive_timevalue));
  
  GST_WRITE_UINT32_BE(ret + 24, gst_clock_time_to_wc_timestamp_seconds(packet->transmit_timevalue));
  GST_WRITE_UINT32_BE(ret + 28, gst_clock_time_to_wc_timestamp_fraction(packet->transmit_timevalue));

  return ret;
}

/**
 * gst_dvb_css_wc_packet_receive:
 * @socket: socket to receive the time packet on
 * @src_address: (out): address of variable to return sender address
 * @error: return address for a #GError, or NULL
 *
 * Receives a #GstDvbCssWcPacket over a socket. Handles interrupted system
 * calls, but otherwise returns NULL on error.
 *
 * Returns: (transfer full): a new #GstDvbCssWcPacket, or NULL on error. Free
 *    with gst_dvb_css_wc_packet_free() when done.
 */
GstDvbCssWcPacket *
gst_dvb_css_wc_packet_receive (GSocket * socket,
    GSocketAddress ** src_address, GError ** error)
{
  gchar buffer[GST_DVB_CSS_WC_PACKET_SIZE];
  GError *err = NULL;
  gssize ret;

  g_return_val_if_fail (G_IS_SOCKET (socket), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  while (TRUE) {
    ret = g_socket_receive_from (socket, src_address, buffer,
        GST_DVB_CSS_WC_PACKET_SIZE, NULL, &err);

    if (ret < 0) {
      if (err->code == G_IO_ERROR_WOULD_BLOCK) {
        g_error_free (err);
        err = NULL;
        continue;
      } else {
        goto receive_error;
      }
    } else if (ret < GST_DVB_CSS_WC_PACKET_SIZE) {
      goto short_packet;
    } else {
      GstDvbCssWcPacket *packet = gst_dvb_css_wc_packet_new ((const guint8 *) buffer);
      PACKET_LOG("Received packet", packet);
      return packet;
    }
  }

receive_error:
  {
    GST_DEBUG ("receive error: %s", err->message);
    g_propagate_error (error, err);
    return NULL;
  }
short_packet:
  {
    GST_DEBUG ("someone sent us a short packet (%" G_GSSIZE_FORMAT " < %d)",
        ret, GST_DVB_CSS_WC_PACKET_SIZE);
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
        "short time packet (%d < %d)", (int) ret, GST_DVB_CSS_WC_PACKET_SIZE);
    return NULL;
  }
}

/**
 * gst_dvb_css_wc_packet_send:
 * @packet: the #GstDvbCssWcPacket to send
 * @socket: socket to send the time packet on
 * @dest_address: address to send the time packet to
 * @error: return address for a #GError, or NULL
 *
 * Sends a #GstDvbCssWcPacket over a socket.
 *
 * MT safe.
 *
 * Returns: TRUE if successful, FALSE in case an error occurred.
 */
gboolean
gst_dvb_css_wc_packet_send (const GstDvbCssWcPacket * packet,
    GSocket * socket, GSocketAddress * dest_address, GError ** error)
{
  gboolean was_blocking;
  guint8 *buffer;
  gssize res;

  g_return_val_if_fail (packet != NULL, FALSE);
  g_return_val_if_fail (G_IS_SOCKET (socket), FALSE);
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (dest_address), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  was_blocking = g_socket_get_blocking (socket);

  if (was_blocking)
    g_socket_set_blocking (socket, FALSE);

  PACKET_LOG("Sending packet", packet);
  /* FIXME: avoid pointless alloc/free, serialise into stack-allocated buffer */
  buffer = gst_dvb_css_wc_packet_serialize (packet);

  res = g_socket_send_to (socket, dest_address, (const gchar *) buffer,
      GST_DVB_CSS_WC_PACKET_SIZE, NULL, error);

  /* datagram packets should be sent as a whole or not at all */
  g_assert (res < 0 || res == GST_DVB_CSS_WC_PACKET_SIZE);

  g_free (buffer);

  if (was_blocking)
    g_socket_set_blocking (socket, TRUE);

  return (res == GST_DVB_CSS_WC_PACKET_SIZE);
}

gint8 gst_dvb_css_wc_packet_encode_precision(gdouble precisionSecs)
{        
    return (gint8)ceil(log2((precisionSecs)));
}

gdouble gst_dvb_css_wc_packet_decode_precision(gint8 precision)
{
    return pow(2, (gdouble)precision);
}

guint32 gst_dvb_css_wc_packet_encode_max_freq_error(gdouble max_freq_error_ppm)
{    
    return (guint32)ceil(max_freq_error_ppm*256);
}

gdouble gst_dvb_css_wc_packet_decode_max_freq_error(guint32 max_freq_error)
{
    return (gdouble)max_freq_error/256;
}

/* GStreamer
 * Copyright (C) 2005 Andy Wingo <wingo@pobox.com>
 *
 * gstnettimeprovider.c: Unit test for the network time provider
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

#include <gst/check/gstcheck.h>
#include <gst/net/gstnet.h>

#include <unistd.h>
#include <math.h>

//#include "gstdvbcsswcserver.h"

GST_START_TEST (test_refcounts)
{
  GstDvbCssWcServer *wc;
  GstClock *clock;

  clock = gst_system_clock_obtain ();
  fail_unless (clock != NULL, "failed to get system clock");

  /* one for gstreamer, one for us */
  ASSERT_OBJECT_REFCOUNT (clock, "system clock", 2);

  wc = gst_dvb_css_wc_server_new (clock, "127.0.0.1", 37034, FALSE, 500);
  fail_unless (wc != NULL, "failed to create dvb css wc server");

  /* one for ntp, one for gstreamer, one for us */
  ASSERT_OBJECT_REFCOUNT (clock, "system clock", 3);
  /* one for us */
  ASSERT_OBJECT_REFCOUNT (wc, "dvb css wc server", 1);

  gst_object_unref (wc);
  ASSERT_OBJECT_REFCOUNT (clock, "dvb css wc server", 2);

  gst_object_unref (clock);
}

GST_END_TEST;

GST_START_TEST (test_packet)
{  
  GstDvbCssWcPacket *packet;
  guint8* buf;
  
  packet = gst_dvb_css_wc_packet_new (NULL);
  fail_unless (packet != NULL, "failed to create packet");  
  
  packet->message_type = 1;
  packet->precision = 2;
  packet->max_freq_error = 3;
  packet->originate_timevalue_secs = 4;
  packet->originate_timevalue_nanos = 5;
  packet->receive_timevalue = 320000000009;
  packet->transmit_timevalue = 7;
  gst_dvb_css_wc_packet_print(packet);
  buf = gst_dvb_css_wc_packet_serialize(packet);      
  g_free (packet);
  
  packet = gst_dvb_css_wc_packet_new (buf);
  gst_dvb_css_wc_packet_print(packet);
  fail_unless (packet != NULL, "failed to create packet");  
  
  fail_unless(packet->message_type == 1, "Wrong type");
  fail_unless(packet->precision == 2, "Wrong precision");
  fail_unless(packet->max_freq_error == 3, "Wrong max_freq_error");
  fail_unless(packet->originate_timevalue_secs == 4, "Wrong originate_timevalue_secs");
  fail_unless(packet->originate_timevalue_nanos == 5, "Wrong originate_timevalue_nanos");
  fail_unless(packet->receive_timevalue == 320000000009, "Wrong receive_timevalue");
  fail_unless(packet->transmit_timevalue == 7, "Wrong transmit_timevalue");
  g_free (packet);

  fail_unless(gst_dvb_css_wc_packet_encode_precision(pow(2, -128)) == -128, "encode precision failed");
  fail_unless(gst_dvb_css_wc_packet_encode_precision(pow(2, 127)) == 127, "encode precision failed");
  fail_unless(gst_dvb_css_wc_packet_encode_precision(0.00001) == -16, "encode precision failed");
  fail_unless(gst_dvb_css_wc_packet_encode_precision(0.0007) == -10, "encode precision failed");
  fail_unless(gst_dvb_css_wc_packet_encode_precision(0.001) == -9, "encode precision failed");
    
  fail_unless(gst_dvb_css_wc_packet_decode_precision(-128) == pow(2, -128), "decode precision failed");
  fail_unless(gst_dvb_css_wc_packet_decode_precision(127) == pow(2, 127), "decode precision failed");
  fail_unless(gst_dvb_css_wc_packet_decode_precision(-16) == pow(2, -16), "decode precision failed");
  fail_unless(gst_dvb_css_wc_packet_decode_precision(-10) == pow(2, -10), "decode precision failed");
  fail_unless(gst_dvb_css_wc_packet_decode_precision(-9) == pow(2, -9), "decode precision failed");
  
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(50) == 12800, "encode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(1900) == 486400, "encode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(0.01) == 3, "encode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(28) == 7168, "encode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(100000) == 25600000, "encode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_encode_max_freq_error(0) == 0, "encode mfe failed");
  
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(12800) == 50, "decode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(486400) == 1900, "decode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(3) == 0.01171875, "decode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(7168) == 28, "decode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(25600000) == 100000, "decode mfe failed");
  fail_unless(gst_dvb_css_wc_packet_decode_max_freq_error(0) == 0, "decode mfe failed");
  
  
}

GST_END_TEST

GST_START_TEST (test_functioning)
{
  GstDvbCssWcServer *wc;
  GstDvbCssWcPacket *packet;
  GstClock *clock;
  guint32 local_secs, local_nanos;
  GSocketAddress *server_addr;
  GInetAddress *addr;
  GSocket *socket;
  gint port = -1;
  GstClockTime prev_receive_timevalue, prev_transmit_timevalue;

  clock = gst_system_clock_obtain ();
  fail_unless (clock != NULL, "failed to get system clock");
  wc = gst_dvb_css_wc_server_new (clock, "127.0.0.1", 37034, TRUE, 500);
  fail_unless (wc != NULL, "failed to create dvb css wc server");

  g_object_get (wc, "port", &port, NULL);
  fail_unless (port > 0);

  socket = g_socket_new (G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_DATAGRAM,
      G_SOCKET_PROTOCOL_UDP, NULL);
  fail_unless (socket != NULL, "could not create socket");

  addr = g_inet_address_new_from_string ("127.0.0.1");
  server_addr = g_inet_socket_address_new (addr, port);
  g_object_unref (addr);

  packet = gst_dvb_css_wc_packet_new (NULL);
  fail_unless (packet != NULL, "failed to create packet");

  packet->message_type = GST_DVB_CSS_WC_MSG_REQUEST;  
  packet->originate_timevalue_secs = local_secs = 123;  
  packet->originate_timevalue_nanos = local_nanos = 456;  

  fail_unless (gst_dvb_css_wc_packet_send (packet, socket, server_addr, NULL));

  g_free (packet);
  
  packet = gst_dvb_css_wc_packet_receive (socket, NULL, NULL);
  fail_unless (packet != NULL, "failed to receive packet");
  fail_unless (packet->message_type == GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP, "wrong msg type");
  fail_unless (packet->originate_timevalue_secs == local_secs, "originate_timevalue_secs is not the same");
  fail_unless (packet->originate_timevalue_nanos == local_nanos, "originate_timevalue_nanos is not the same");
  fail_unless (packet->receive_timevalue < packet->transmit_timevalue, "remote time not after local time");
  fail_unless (packet->transmit_timevalue < gst_clock_get_time (clock), "remote time in the future");

  prev_receive_timevalue = packet->receive_timevalue;
  prev_transmit_timevalue = packet->transmit_timevalue;
  
  g_free (packet);

  packet = gst_dvb_css_wc_packet_receive (socket, NULL, NULL);  
  fail_unless (packet != NULL, "failed to receive packet");
  fail_unless (packet->message_type == GST_DVB_CSS_WC_MSG_FOLLOWUP, "wrong msg type");
  fail_unless (packet->originate_timevalue_secs == local_secs, "originate_timevalue_secs is not the same");
  fail_unless (packet->originate_timevalue_nanos == local_nanos, "originate_timevalue_nanos is not the same");
  fail_unless (packet->receive_timevalue < packet->transmit_timevalue, "remote time not after local time");
  fail_unless (packet->transmit_timevalue < gst_clock_get_time (clock), "remote time in the future");
  
  fail_unless (prev_receive_timevalue == packet->receive_timevalue, "remote time not after local time");
  fail_unless (prev_transmit_timevalue < packet->transmit_timevalue, "remote time not after local time");
   
  g_free (packet);
    
  
  
  packet = gst_dvb_css_wc_packet_new (NULL);
  fail_unless (packet != NULL, "failed to create packet");
  packet->message_type = GST_DVB_CSS_WC_MSG_REQUEST;  
  fail_unless (gst_dvb_css_wc_packet_send (packet, socket, server_addr, NULL));
  g_free (packet);
  
  packet = gst_dvb_css_wc_packet_new (NULL);
  fail_unless (packet != NULL, "failed to create packet");
  packet->message_type = GST_DVB_CSS_WC_MSG_RESPONSE;    //wrong msg ignored by server
  fail_unless (gst_dvb_css_wc_packet_send (packet, socket, server_addr, NULL));
  g_free (packet);
  
  packet = gst_dvb_css_wc_packet_receive (socket, NULL, NULL);  
  fail_unless (packet != NULL, "failed to receive packet");
  fail_unless (packet->message_type == GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP, "wrong msg type");
  g_free (packet);
  
  g_object_unref (socket);
  g_object_unref (server_addr);

  gst_object_unref (wc);
  gst_object_unref (clock);
}

GST_END_TEST;

static Suite *
gst_net_time_provider_suite (void)
{
  Suite *s = suite_create ("GstNetTimeProvider");
  TCase *tc_chain = tcase_create ("generic tests");

  suite_add_tcase (s, tc_chain);
  tcase_add_test (tc_chain, test_refcounts);
  tcase_add_test (tc_chain, test_packet);
  tcase_add_test (tc_chain, test_functioning);

  return s;
}

GST_CHECK_MAIN (gst_net_time_provider);

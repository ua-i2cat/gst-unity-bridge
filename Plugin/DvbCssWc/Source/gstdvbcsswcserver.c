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

#include "gstdvbcsswcserver.h"
#include "gstdvbcsswcpacket.h"
#include "gstdvbcsswccommon.h"
#include <stdlib.h>

GST_DEBUG_CATEGORY_STATIC (dvbcss_wc_server);
#define GST_CAT_DEFAULT (dvbcss_wc_server)

#define DEFAULT_ADDRESS         "0.0.0.0"
#define DEFAULT_PORT            5637

#define IS_ACTIVE(self) (g_atomic_int_get (&((self)->priv->active)))

enum
{
  PROP_0,
  PROP_PORT,
  PROP_ADDRESS,
  PROP_CLOCK,
  PROP_ACTIVE,
  PROP_FOLLOWUP,
  PROP_MAX_FREQ_ERROR_PPM
};

#define GST_DVB_CSS_WC_SERVER_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_DVB_CSS_WC_SERVER, GstDvbCssWcServerPrivate))

struct _GstDvbCssWcServerPrivate
{
  gchar *address;
  int port;

  GThread *thread;

  GstClock *clock;

  gboolean active;              /* ATOMIC */

  GSocket *socket;
  GCancellable *cancel;
  gboolean made_cancel_fd;

  gboolean followup;
  gdouble precision_secs;
  guint32 max_freq_error_ppm;
};
static void gst_dvb_css_wc_server_initable_iface_init (gpointer g_iface);

static gboolean gst_dvb_css_wc_server_start (GstDvbCssWcServer * bself, GError ** error);
static void gst_dvb_css_wc_server_stop (GstDvbCssWcServer * bself);

static gpointer gst_dvb_css_wc_server_thread (gpointer data);

static void gst_dvb_css_wc_server_finalize (GObject * object);
static void gst_dvb_css_wc_server_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dvb_css_wc_server_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

#define _do_init \
  GST_DEBUG_CATEGORY_INIT (dvbcss_wc_server, "dvbcss_wc_server", 0, "Network dvb css wc server"); \
  G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, gst_dvb_css_wc_server_initable_iface_init)

#define gst_dvb_css_wc_server_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstDvbCssWcServer, gst_dvb_css_wc_server,
    GST_TYPE_OBJECT, _do_init);

static void
gst_dvb_css_wc_server_class_init (GstDvbCssWcServerClass * klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  g_assert (sizeof (GstClockTime) == 8);

  g_type_class_add_private (klass, sizeof (GstDvbCssWcServerPrivate));

  gobject_class->finalize = gst_dvb_css_wc_server_finalize;
  gobject_class->set_property = gst_dvb_css_wc_server_set_property;
  gobject_class->get_property = gst_dvb_css_wc_server_get_property;

  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "port",
          "The port to receive the packets from, 0=allocate", 0, G_MAXUINT16,
          DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ADDRESS,
      g_param_spec_string ("address", "address",
          "The address to bind on, as a dotted quad (x.x.x.x)", DEFAULT_ADDRESS,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_CLOCK,
      g_param_spec_object ("clock", "Clock",
          "The clock to export over the network", GST_TYPE_CLOCK,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ACTIVE,
      g_param_spec_boolean ("active", "Active",
          "TRUE if the clock will respond to queries over the network", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FOLLOWUP,
      g_param_spec_boolean ("followup", "Followup",
          "TRUE if the followup msg will be send", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_MAX_FREQ_ERROR_PPM,
      g_param_spec_uint ("max_freq_error_ppm", "max_freq_error_ppm",
          "max freq error ppm", 0, G_MAXUINT32,
          0,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gst_dvb_css_wc_server_init (GstDvbCssWcServer * self)
{
  self->priv = GST_DVB_CSS_WC_SERVER_GET_PRIVATE (self);

  self->priv->port = DEFAULT_PORT;
  self->priv->address = g_strdup (DEFAULT_ADDRESS);
  self->priv->thread = NULL;
  self->priv->active = TRUE;
  self->priv->followup = FALSE;
  self->priv->precision_secs = 0;
  self->priv->max_freq_error_ppm = 0;
}

static void
gst_dvb_css_wc_server_finalize (GObject * object)
{
  GstDvbCssWcServer *self = GST_DVB_CSS_WC_SERVER (object);

  if (self->priv->thread) {
    gst_dvb_css_wc_server_stop (self);
    g_assert (self->priv->thread == NULL);
  }

  g_free (self->priv->address);
  self->priv->address = NULL;

  if (self->priv->clock)
    gst_object_unref (self->priv->clock);
  self->priv->clock = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gpointer
gst_dvb_css_wc_server_thread (gpointer data)
{
  GstDvbCssWcServer *self = data;
  GCancellable *cancel = self->priv->cancel;
  GSocket *socket = self->priv->socket;
  GstDvbCssWcPacket *packet, *reply, *followupReply;
  GError *err = NULL;
  GstClock *clock = self->priv->clock;
  GstClockTime time;

  self->priv->precision_secs = measure_precision_sec (clock);

  GST_TRACE_OBJECT (self, "dvb css wc server thread is running");

  while (TRUE) {
    GSocketAddress *sender_addr = NULL;

    GST_TRACE_OBJECT (self, "waiting on socket");
    if (!g_socket_condition_wait (socket, G_IO_IN, cancel, &err)) {
      if (err->code == G_IO_ERROR_CANCELLED)
        break;
      
      GST_WARNING_OBJECT (self, "socket error: %s", err->message);

      /* try again */
      g_usleep (G_USEC_PER_SEC / 10);
      g_error_free (err);
      err = NULL;
      continue;
    }

    /* got data in */
    time = gst_clock_get_time(clock);
    packet = gst_dvb_css_wc_packet_receive (socket, &sender_addr, &err);
    
    if (err != NULL) {
      GST_WARNING_OBJECT (self, "receive error: %s", err->message);
      g_usleep (G_USEC_PER_SEC / 10);
      g_error_free (err);
      err = NULL;
      continue;
    }
    
    if (IS_ACTIVE (self)) {
        if(packet->message_type == GST_DVB_CSS_WC_MSG_REQUEST
           && packet->version == 0){
        /* do what we were asked to and send the packet back */
            reply = gst_dvb_css_wc_packet_copy(packet);
            reply->receive_timevalue = time;

            if(self->priv->followup)
                reply->message_type = GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP;
            else
                reply->message_type = GST_DVB_CSS_WC_MSG_RESPONSE;               
            reply->precision = gst_dvb_css_wc_packet_encode_precision(self->priv->precision_secs);
            reply->max_freq_error = gst_dvb_css_wc_packet_encode_max_freq_error(self->priv->max_freq_error_ppm);;
            reply->transmit_timevalue = gst_clock_get_time(clock);
            gst_dvb_css_wc_packet_send (reply, socket, sender_addr, NULL);

            if(self->priv->followup){
                followupReply = gst_dvb_css_wc_packet_copy(reply);
                followupReply->message_type = GST_DVB_CSS_WC_MSG_FOLLOWUP;
                // FIXME: transmit_timevalue should be set to more accurate
                // measurement of the time of transmission of the previous response
                //followupReply->transmit_timevalue = gst_clock_get_time(clock);
                gst_dvb_css_wc_packet_send (followupReply, socket, sender_addr, NULL);
                gst_dvb_css_wc_packet_free (followupReply);
            }
            gst_dvb_css_wc_packet_free (reply);
        }else
            GST_ERROR_OBJECT(self, "Received non request message");     
    }    
    g_object_unref (sender_addr);
    gst_dvb_css_wc_packet_free (packet);
  }

  g_error_free (err);

  GST_TRACE_OBJECT (self, "dvb css wc server thread is stopping");
  return NULL;
}

static void
gst_dvb_css_wc_server_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDvbCssWcServer *self = GST_DVB_CSS_WC_SERVER (object);
  GstClock **clock_p = &self->priv->clock;

  switch (prop_id) {
    case PROP_PORT:
      self->priv->port = g_value_get_int (value);
      break;
    case PROP_ADDRESS:
      g_free (self->priv->address);
      if (g_value_get_string (value) == NULL)
        self->priv->address = g_strdup (DEFAULT_ADDRESS);
      else
        self->priv->address = g_strdup (g_value_get_string (value));
      break;
    case PROP_CLOCK:
      gst_object_replace ((GstObject **) clock_p,
          (GstObject *) g_value_get_object (value));
      break;
    case PROP_ACTIVE:
      g_atomic_int_set (&self->priv->active, g_value_get_boolean (value));
      break;
    case PROP_FOLLOWUP:
      g_atomic_int_set (&self->priv->followup, g_value_get_boolean (value));
      break;
    case PROP_MAX_FREQ_ERROR_PPM:
      self->priv->max_freq_error_ppm = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dvb_css_wc_server_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDvbCssWcServer *self = GST_DVB_CSS_WC_SERVER (object);

  switch (prop_id) {
    case PROP_PORT:
      g_value_set_int (value, self->priv->port);
      break;
    case PROP_ADDRESS:
      g_value_set_string (value, self->priv->address);
      break;
    case PROP_CLOCK:
      g_value_set_object (value, self->priv->clock);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, IS_ACTIVE (self));
      break;
    case PROP_FOLLOWUP:
      g_value_set_boolean (value, self->priv->followup);
      break;
    case PROP_MAX_FREQ_ERROR_PPM:
      g_value_set_uint (value, self->priv->max_freq_error_ppm);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_dvb_css_wc_server_start (GstDvbCssWcServer * self, GError ** error)
{    
  GSocketAddress *socket_addr, *bound_addr;
  GInetAddress *inet_addr;
  GPollFD dummy_pollfd;
  GSocket *socket;  
  int port;
  gchar *address;
  GError *err = NULL;
  
  if (self->priv->address) {
    inet_addr = g_inet_address_new_from_string (self->priv->address);
    if (inet_addr == NULL) {
      err =
          g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
          "Failed to parse address '%s'", self->priv->address);
      goto invalid_address;
  }
  } else {
    inet_addr = g_inet_address_new_any (G_SOCKET_FAMILY_IPV4);
  }

  GST_TRACE_OBJECT (self, "creating socket");
  socket = g_socket_new (g_inet_address_get_family (inet_addr),
      G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &err);

  if (!socket)
    goto no_socket;

  GST_TRACE_OBJECT (self, "binding on port %d", self->priv->port);
  socket_addr = g_inet_socket_address_new (inet_addr, self->priv->port);
  if (!g_socket_bind (socket, socket_addr, TRUE, &err)) {
    g_object_unref (socket_addr);
    g_object_unref (inet_addr);
    goto bind_error;
  }
  g_object_unref (socket_addr);
  g_object_unref (inet_addr);

  bound_addr = g_socket_get_local_address (socket, NULL);
  port = g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (bound_addr));
  inet_addr =
      g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (bound_addr));
  address = g_inet_address_to_string (inet_addr);

  if (g_strcmp0 (address, self->priv->address)) {
    g_free (self->priv->address);
    self->priv->address = address;
    GST_TRACE_OBJECT (self, "notifying address %s", address);
    g_object_notify (G_OBJECT (self), "address");
  } else {
    g_free (address);
  }
  if (port != self->priv->port) {
    self->priv->port = port;
    GST_TRACE_OBJECT (self, "notifying port %d", port);
    g_object_notify (G_OBJECT (self), "port");
  }
  GST_DEBUG_OBJECT (self, "bound on UDP address %s, port %d",
      self->priv->address, port);
  g_object_unref (bound_addr);

  self->priv->socket = socket;
  self->priv->cancel = g_cancellable_new ();
  self->priv->made_cancel_fd =
      g_cancellable_make_pollfd (self->priv->cancel, &dummy_pollfd);

  self->priv->thread = g_thread_try_new ("GstDvbCssWcServer",
      gst_dvb_css_wc_server_thread, self, &err);

  if (!self->priv->thread)
    goto no_thread;

  return TRUE;

  /* ERRORS */
invalid_address:
  {
    GST_ERROR_OBJECT (self, "invalid address: %s", self->priv->address);
    g_propagate_error (error, err);
    return FALSE;
  }
no_socket:
  {
    GST_ERROR_OBJECT (self, "could not create socket: %s", err->message);
    g_propagate_error (error, err);
    g_object_unref (inet_addr);
    return FALSE;
  }
bind_error:
  {
    GST_ERROR_OBJECT (self, "bind failed: %s", err->message);
    g_propagate_error (error, err);
    g_object_unref (socket);
    return FALSE;
  }
no_thread:
  {
    GST_ERROR_OBJECT (self, "could not create thread: %s", err->message);
    g_propagate_error (error, err);
    g_object_unref (self->priv->socket);
    self->priv->socket = NULL;
    g_object_unref (self->priv->cancel);
    self->priv->cancel = NULL;
    return FALSE;
  }
}

static void
gst_dvb_css_wc_server_stop (GstDvbCssWcServer * self)
{
  g_return_if_fail (self->priv->thread != NULL);

  GST_TRACE_OBJECT (self, "stopping..");
  g_cancellable_cancel (self->priv->cancel);

  g_thread_join (self->priv->thread);
  self->priv->thread = NULL;

  if (self->priv->made_cancel_fd)
    g_cancellable_release_fd (self->priv->cancel);

  g_object_unref (self->priv->cancel);
  self->priv->cancel = NULL;

  g_object_unref (self->priv->socket);
  self->priv->socket = NULL;

  GST_DEBUG_OBJECT (self, "stopped");
}

static gboolean
gst_dvb_css_wc_server_initable_init (GInitable * initable,
    GCancellable * cancellable, GError ** error)
{
  GstDvbCssWcServer *self = GST_DVB_CSS_WC_SERVER (initable);

  return gst_dvb_css_wc_server_start (self, error);
}

static void
gst_dvb_css_wc_server_initable_iface_init (gpointer g_iface)
{
  GInitableIface *iface = g_iface;

  iface->init = gst_dvb_css_wc_server_initable_init;
}

/**
 * gst_dvb_css_wc_server_new:
 * @clock: a #GstClock to export over the network
 * @address: (allow-none): an address to bind on as a dotted quad
 *           (xxx.xxx.xxx.xxx), IPv6 address, or NULL to bind to all addresses
 * @port: a port to bind on, or 0 to let the kernel choose
 *TODO
 * Allows network clients to get the current time of @clock.
 *
 * Returns: the new #GstDvbCssWcServer, or NULL on error
 */
GstDvbCssWcServer *
gst_dvb_css_wc_server_new (GstClock * clock, const gchar * address, gint port, gboolean followup, guint32 max_freq_error_ppm)
{
  GstDvbCssWcServer *ret;
  
  g_return_val_if_fail (clock && GST_IS_CLOCK (clock), NULL);
  g_return_val_if_fail (port >= 0 && port <= G_MAXUINT16, NULL);

  ret =
      g_initable_new (GST_TYPE_DVB_CSS_WC_SERVER, NULL, NULL, "clock", clock,
      "address", address, "port", port, "followup", followup, "max_freq_error_ppm", max_freq_error_ppm, NULL);

  return ret;
}

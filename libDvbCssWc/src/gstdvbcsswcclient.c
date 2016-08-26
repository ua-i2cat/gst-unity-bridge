/* GStreamer
 * Copyright (C) 2016
 *  Authors:  Wojciech Kapsa PSNC <kapsa@man.poznan.pl>
 *  Authors:  Xavi Artigas <xavi.artigas@i2cat.net>
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

#include "gstdvbcsswcpacket.h"
#include "gstdvbcsswcclient.h"

#include <gio/gio.h>

#include <string.h>
#include <glib-2.0/glib/gmem.h>
#include <glib-2.0/glib/gmessages.h>
#include <stdlib.h>

#define DEFAULT_ADDRESS         "127.0.0.1"
#define DEFAULT_PORT            5637
#define DEFAULT_TIMEOUT         GST_SECOND
#define DEFAULT_BASE_TIME       0
#define DEFAULT_MAX_FREQ_ERR_PPM 500
#define DEFAULT_SOCKET_TIMEOUT 1000

GST_DEBUG_CATEGORY_STATIC (wc_client_debug);
#define GST_CAT_DEFAULT (wc_client_debug)

enum
{
  PROP_0,
  PROP_ADDRESS,
  PROP_PORT,
  PROP_ROUNDTRIP_LIMIT,
  PROP_MINIMUM_UPDATE_INTERVAL,
  PROP_BUS,
  PROP_BASE_TIME,
  PROP_INTERNAL_CLOCK,
  PROP_IS_NTP
};

#define GST_DVB_CSS_WC_CLIENT_CLOCK_GET_PRIVATE(obj)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, GstDvbCssWcClientClockPrivate))

typedef struct candidate Candidate;

struct _GstDvbCssWcClientClockPrivate
{
  GstClock *internal_clock;

  GstClockTime base_time, internal_base_time;

  GSocketAddress *servaddr;
  
  gchar *address;
  gint port;

  GstBus *bus;
  
  GThread *thread;
  
  GSocket *socket;
  GCancellable *cancel;
  gboolean made_cancel_fd;
  gint64 socket_timeout;
  
  gulong synced_id;
  
  //algorithm specific
  Candidate *best_candidate;
  guint32 max_freq_error_ppm;
};

#define _do_init \
  GST_DEBUG_CATEGORY_INIT (wc_client_debug, "wc_client_debug", 0, "DVB CSS WC client clock");
G_DEFINE_TYPE_WITH_CODE (GstDvbCssWcClientClock,
    gst_dvb_css_wc_client_clock, GST_TYPE_SYSTEM_CLOCK, _do_init);

static void gst_dvb_css_wc_client_clock_finalize (GObject * object);
static void gst_dvb_css_wc_client_clock_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dvb_css_wc_client_clock_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_dvb_css_wc_client_clock_constructed (GObject * object);

static gboolean gst_dvb_css_wc_client_clock_start (GstDvbCssWcClientClock *
    self);
static void gst_dvb_css_wc_client_clock_stop (GstDvbCssWcClientClock *
    self);
static gpointer gst_dvb_css_wc_client_clock_thread (gpointer data);

static GstClockTime gst_dvb_css_wc_client_clock_get_internal_time (GstClock * clock);

gdouble gst_dvb_css_wc_client_measure_precision(GstClock *clock);
Candidate* candidate_new(GstDvbCssWcPacket *msg, GstClockTime time);
GstClockTime calc_dispersion(GstDvbCssWcClientClock *self, Candidate *candidate);
gboolean gst_dvb_css_wc_client_clock_send_request(gpointer data);
GstDvbCssWcPacket *gst_dvb_css_wc_client_clock_receive_msg(gpointer data);

static void
gst_dvb_css_wc_client_clock_class_init (GstDvbCssWcClientClockClass * klass)
{
  GObjectClass *gobject_class;
  GstClockClass *clock_class;

  gobject_class = G_OBJECT_CLASS (klass);
  clock_class = GST_CLOCK_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GstDvbCssWcClientClockPrivate));

  gobject_class->finalize = gst_dvb_css_wc_client_clock_finalize;
  gobject_class->get_property = gst_dvb_css_wc_client_clock_get_property;
  gobject_class->set_property = gst_dvb_css_wc_client_clock_set_property;
  gobject_class->constructed = gst_dvb_css_wc_client_clock_constructed;

  g_object_class_install_property (gobject_class, PROP_ADDRESS,
      g_param_spec_string ("address", "address",
          "The IP address of the machine providing a time server",
          DEFAULT_ADDRESS,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PORT,
      g_param_spec_int ("port", "port",
          "The port on which the remote server is listening", 0, G_MAXUINT16,
          DEFAULT_PORT,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BUS,
      g_param_spec_object ("bus", "bus",
          "A GstBus on which to send clock status information", GST_TYPE_BUS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BASE_TIME,
      g_param_spec_uint64 ("base-time", "Base Time",
          "Initial time that is reported before synchronization", 0,
          G_MAXUINT64, DEFAULT_BASE_TIME,
          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_INTERNAL_CLOCK,
      g_param_spec_object ("internal-clock", "Internal Clock",
          "Internal clock that directly slaved to the remote clock",
          GST_TYPE_CLOCK, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  clock_class->get_internal_time = gst_dvb_css_wc_client_clock_get_internal_time;
}

static void
gst_dvb_css_wc_client_clock_init (GstDvbCssWcClientClock * self)
{
  GstDvbCssWcClientClockPrivate *priv;
  GstClock *clock;
  
  self->priv = priv = GST_DVB_CSS_WC_CLIENT_CLOCK_GET_PRIVATE (self);

  GST_OBJECT_FLAG_SET (self, GST_CLOCK_FLAG_CAN_SET_MASTER);
  GST_OBJECT_FLAG_SET (self, GST_CLOCK_FLAG_NEEDS_STARTUP_SYNC);

  priv->port = DEFAULT_PORT;
  priv->address = g_strdup (DEFAULT_ADDRESS); 

  clock = gst_system_clock_obtain ();
  priv->base_time = DEFAULT_BASE_TIME;
  priv->internal_base_time = gst_clock_get_time (clock);
  gst_object_unref (clock);
  
  priv->max_freq_error_ppm = DEFAULT_MAX_FREQ_ERR_PPM;
  priv->best_candidate = NULL;
  priv->socket_timeout = DEFAULT_SOCKET_TIMEOUT;
}

static GstClockTime
gst_dvb_css_wc_client_clock_get_internal_time (GstClock * clock)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (clock);

  if (!gst_clock_is_synced (self->priv->internal_clock)) {
    GstClockTime now = gst_clock_get_internal_time (self->priv->internal_clock);
    return gst_clock_adjust_with_calibration (self->priv->internal_clock, now,
        self->priv->internal_base_time, self->priv->base_time, 1, 1);
  }

  return gst_clock_get_time (self->priv->internal_clock);
}

static void
gst_dvb_css_wc_client_clock_finalize (GObject * object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);

  if (self->priv->thread) {
    gst_dvb_css_wc_client_clock_stop (self);
  }
  
  if (self->priv->synced_id)
    g_signal_handler_disconnect (self->priv->internal_clock,
        self->priv->synced_id);
  self->priv->synced_id = 0; 

  g_free (self->priv->address);
  self->priv->address = NULL;
  
  if (self->priv->socket != NULL) {
    if (!g_socket_close (self->priv->socket, NULL))
      GST_ERROR_OBJECT (self, "Failed to close socket");
    g_object_unref (self->priv->socket);
    self->priv->socket = NULL;
  }

  if (self->priv->bus != NULL) {
    gst_object_unref (self->priv->bus);
    self->priv->bus = NULL;
  }

  G_OBJECT_CLASS (gst_dvb_css_wc_client_clock_parent_class)->finalize (object);
}

static void
gst_dvb_css_wc_client_clock_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);

  switch (prop_id) {
    case PROP_ADDRESS:
      GST_OBJECT_LOCK (self);
      g_free (self->priv->address);
      self->priv->address = g_value_dup_string (value);
      if (self->priv->address == NULL)
        self->priv->address = g_strdup (DEFAULT_ADDRESS);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_PORT:
      GST_OBJECT_LOCK (self);
      self->priv->port = g_value_get_int (value);
      GST_OBJECT_UNLOCK (self);
      break;    
    case PROP_BUS:
      GST_OBJECT_LOCK (self);
      if (self->priv->bus)
        gst_object_unref (self->priv->bus);
      self->priv->bus = g_value_dup_object (value);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_BASE_TIME:{
      GstClock *clock;

      self->priv->base_time = g_value_get_uint64 (value);
      clock = gst_system_clock_obtain ();
      self->priv->internal_base_time = gst_clock_get_time (clock);
      gst_object_unref (clock);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_dvb_css_wc_client_clock_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);

  switch (prop_id) {
    case PROP_ADDRESS:
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->priv->address);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_PORT:
      g_value_set_int (value, self->priv->port);
      break;    
    case PROP_BUS:
      GST_OBJECT_LOCK (self);
      g_value_set_object (value, self->priv->bus);
      GST_OBJECT_UNLOCK (self);
      break;
    case PROP_BASE_TIME:
      g_value_set_uint64 (value, self->priv->base_time);
      break;
    case PROP_INTERNAL_CLOCK:
      g_value_set_object (value, self->priv->internal_clock);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_dvb_css_wc_client_clock_start (GstDvbCssWcClientClock * self)
{
  GSocketAddress *servaddr;
  GSocketAddress *myaddr;
  GSocketAddress *anyaddr;
  GInetAddress *inetaddr;
  GSocket *socket;
  GError *error = NULL;
  GSocketFamily family;
  GPollFD dummy_pollfd;  
  GError *err = NULL;

  g_return_val_if_fail (self->priv->address != NULL, FALSE);
  g_return_val_if_fail (self->priv->servaddr == NULL, FALSE);

  /* create target address */
  inetaddr = g_inet_address_new_from_string (self->priv->address);
  if (inetaddr == NULL) {
    err =
          g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
          "Failed to parse address '%s'", self->priv->address);
    goto invalid_address;    
  }

  family = g_inet_address_get_family (inetaddr);

  servaddr = g_inet_socket_address_new (inetaddr, self->priv->port);
  g_object_unref (inetaddr);

  g_assert (servaddr != NULL);

  GST_DEBUG_OBJECT (self, "will communicate with %s:%d", self->priv->address,
      self->priv->port);

  socket = g_socket_new (family, G_SOCKET_TYPE_DATAGRAM,
      G_SOCKET_PROTOCOL_UDP, &error);

  if (socket == NULL)
    goto no_socket;

  GST_DEBUG_OBJECT (self, "binding socket");
  inetaddr = g_inet_address_new_any (family);
  anyaddr = g_inet_socket_address_new (inetaddr, 0);
  g_socket_bind (socket, anyaddr, TRUE, &error);
  g_object_unref (anyaddr);
  g_object_unref (inetaddr);

  if (error != NULL)
    goto bind_error;

  /* check address we're bound to, mostly for debugging purposes */
  myaddr = g_socket_get_local_address (socket, &error);

  if (myaddr == NULL)
    goto getsockname_error;

  GST_DEBUG_OBJECT (self, "socket opened on UDP port %d",
      g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (myaddr)));

  g_object_unref (myaddr);

  self->priv->cancel = g_cancellable_new ();
  self->priv->made_cancel_fd =
      g_cancellable_make_pollfd (self->priv->cancel, &dummy_pollfd);

  self->priv->socket = socket;
  self->priv->servaddr = G_SOCKET_ADDRESS (servaddr);

  self->priv->thread = g_thread_try_new ("GstDvbCssWcClientClock",
      gst_dvb_css_wc_client_clock_thread, self, &error);

  if (error != NULL)
    goto no_thread;

  return TRUE;

  /* ERRORS */
no_socket:
  {
    GST_ERROR_OBJECT (self, "socket_new() failed: %s", error->message);
    g_error_free (error);
    return FALSE;
  }
bind_error:
  {
    GST_ERROR_OBJECT (self, "bind failed: %s", error->message);
    g_error_free (error);
    g_object_unref (socket);
    return FALSE;
  }
getsockname_error:
  {
    GST_ERROR_OBJECT (self, "get_local_address() failed: %s", error->message);
    g_error_free (error);
    g_object_unref (socket);
    return FALSE;
  }
invalid_address:
  {
    GST_ERROR_OBJECT (self, "invalid address: %s %s", self->priv->address,
        err->message);
    g_clear_error (&err);
    g_object_unref(inetaddr);
    return FALSE;
  }
no_thread:
  {
    GST_ERROR_OBJECT (self, "could not create thread: %s", error->message);
    g_object_unref (self->priv->servaddr);
    self->priv->servaddr = NULL;
    g_object_unref (self->priv->socket);
    self->priv->socket = NULL;
    g_error_free (error);
    return FALSE;
  }
}

static void
gst_dvb_css_wc_client_clock_stop (GstDvbCssWcClientClock * self)
{
  if (self->priv->thread == NULL)
    return;

  GST_INFO_OBJECT (self, "stopping...");
  g_cancellable_cancel (self->priv->cancel);

  g_thread_join (self->priv->thread);
  self->priv->thread = NULL;

  if (self->priv->made_cancel_fd)
    g_cancellable_release_fd (self->priv->cancel);

  g_object_unref (self->priv->cancel);
  self->priv->cancel = NULL;

  g_object_unref (self->priv->servaddr);
  self->priv->servaddr = NULL;

  g_object_unref (self->priv->socket);
  self->priv->socket = NULL;

  GST_INFO_OBJECT (self, "stopped");
}

gdouble gst_dvb_css_wc_client_measure_precision(GstClock *clock)
{
    gdouble nsec = gst_guint64_to_gdouble(gst_clock_get_resolution(clock));      
    return GST_TIME_AS_SECONDS(nsec);
}

struct candidate
{
    GstClockTime t1, t2, t3, t4;
    gint32 rtt, offset;
    gdouble precision_secs;
    guint32 max_freq_error_ppm;
    GstDvbCssWcPacket msg;
};

Candidate* candidate_new(GstDvbCssWcPacket *msg, GstClockTime time)
{
    Candidate* ret = g_malloc(sizeof(Candidate));
    ret->t1 = wc_timestamp_to_gst_clock_time(msg->originate_timevalue_secs, msg->originate_timevalue_nanos);    
    ret->t2 = msg->receive_timevalue;
    ret->t3 = msg->transmit_timevalue;
    ret->t4 = time;            
    ret->rtt = (ret->t4 - ret->t1)-(ret->t3 - ret->t2);
    ret->offset = ((ret->t3 + ret->t2)-(ret->t4 + ret->t1))/2;
    ret->precision_secs = gst_dvb_css_wc_packet_decode_precision(msg->precision);
    ret->max_freq_error_ppm = gst_dvb_css_wc_packet_decode_max_freq_error(msg->max_freq_error);
    return ret;
}

GstClockTime
calc_dispersion(GstDvbCssWcClientClock *self, Candidate *candidate)
{
    gdouble local_precision = gst_dvb_css_wc_client_measure_precision(self->priv->internal_clock);

    GstClockTime dispersion = 1000000000*(local_precision + candidate->precision_secs) +
                             (candidate->max_freq_error_ppm*(candidate->t3-candidate->t2) +
                             self->priv->max_freq_error_ppm*(candidate->t4-candidate->t1) +
        (candidate->max_freq_error_ppm+self->priv->max_freq_error_ppm) *
        (gst_clock_get_time(self->priv->internal_clock) -candidate->t4)
        ) / 1000000 + candidate->rtt / 2;

    return dispersion;
}

static void
gst_dvb_css_wc_client_clock_update(GstDvbCssWcClientClock *self, GstDvbCssWcPacket *pkt, GstClockTime time)
{   
    GstClockTime candidate_dispersion;
    Candidate *candidate = NULL;
    GstClockTime current_dispersion = G_MAXUINT64;
    GstClockTime internal = 0, external = 0, rate_num = 0, rate_denom = 0;     
    
    if(pkt == NULL)
        return;
    
    if(pkt->message_type == GST_DVB_CSS_WC_MSG_REQUEST)
        return;
    
    if(self->priv->best_candidate != NULL){
        current_dispersion = calc_dispersion(self, self->priv->best_candidate);
    }
    
    candidate = candidate_new(pkt, time);
    if(candidate != NULL){
        candidate_dispersion = calc_dispersion(self, candidate);
        GST_INFO("Current dispersion: %"G_GUINT64_FORMAT"   Candidate dispersion: %"G_GUINT64_FORMAT"", current_dispersion, candidate_dispersion);        
        if(current_dispersion >= candidate_dispersion){
            GST_INFO("Clock updated. Offset: %" G_GINT32_FORMAT"\n", candidate->offset);            
            if(self->priv->best_candidate != NULL){
                g_free(self->priv->best_candidate);
            }
            self->priv->best_candidate = candidate;           
            gst_clock_get_calibration (self->priv->internal_clock, &internal, &external, &rate_num, &rate_denom);
            gst_clock_set_calibration(self->priv->internal_clock, internal, external + candidate->offset, rate_num, rate_denom);                        
		}
		else {
			g_free(candidate);
		}
    }             
}

gboolean
gst_dvb_css_wc_client_clock_send_request(gpointer data)
{
    GstDvbCssWcClientClock *self = data;
    GstDvbCssWcPacket *req;
    GstClockTime time;
    gboolean success;
    GError *err = NULL;
    
    req = gst_dvb_css_wc_packet_new (NULL);
    req->message_type = GST_DVB_CSS_WC_MSG_REQUEST;

    time = gst_clock_get_time(self->priv->internal_clock);
    req->originate_timevalue_secs = gst_clock_time_to_wc_timestamp_seconds(time);
    req->originate_timevalue_nanos = gst_clock_time_to_wc_timestamp_fraction(time);
    
    GST_DEBUG_OBJECT (self, "sending packet:");
    success = gst_dvb_css_wc_packet_send (req, self->priv->socket, self->priv->servaddr, &err);
    if (err != NULL) {
        GST_DEBUG_OBJECT (self, "request send error: %s", err->message);          
        g_error_free (err);
        err = NULL;
    }
    gst_dvb_css_wc_packet_print(req);
    g_free (req);
    
    return success;
}

GstDvbCssWcPacket *
gst_dvb_css_wc_client_clock_receive_msg(gpointer data)
{
    GstDvbCssWcClientClock *self = data;
    GError *err = NULL;
    GstDvbCssWcPacket *resp;    
    
    GST_TRACE_OBJECT (self, "timeout: %" G_GINT64_FORMAT "us", self->priv->socket_timeout);
    if (!g_socket_condition_timed_wait (self->priv->socket, G_IO_IN, self->priv->socket_timeout,
            self->priv->cancel, &err)) {
      /* cancelled, timeout or error */
      if (err->code == G_IO_ERROR_CANCELLED) {
        GST_INFO_OBJECT (self, "cancelled");
        g_clear_error (&err);
        return NULL;
      }
            
      GST_DEBUG_OBJECT (self, "socket error: %s", err->message);   
      /* try again */
      g_usleep (G_USEC_PER_SEC / 10);
      g_error_free (err);
      err = NULL;
      return NULL;                       
    }

    /* got data in */
    resp = gst_dvb_css_wc_packet_receive (self->priv->socket, &self->priv->servaddr, &err);    
    gst_dvb_css_wc_packet_print(resp);
    if (err != NULL) {
      GST_DEBUG_OBJECT (self, "receive error: %s", err->message);
      g_usleep (G_USEC_PER_SEC / 10);
      g_error_free (err);
      err = NULL;
      return NULL;
    }   
    
    return resp;    
}

static gpointer 
gst_dvb_css_wc_client_clock_thread (gpointer data)
{
  GstDvbCssWcClientClock *self = data;
  GSocket *socket = self->priv->socket;  
  GstDvbCssWcPacket *resp;
  GstClockTime time;
  gboolean success = FALSE;
  guint8 msg_type = GST_DVB_CSS_WC_MSG_REQUEST;

  GST_INFO_OBJECT (self, "dvb client clock thread running, socket=%p", socket);

  g_socket_set_blocking (socket, TRUE);
  g_socket_set_timeout (socket, 0);
  
  while (!g_cancellable_is_cancelled (self->priv->cancel)) {    
    success = gst_dvb_css_wc_client_clock_send_request(data);
    if(success == TRUE){
        resp = gst_dvb_css_wc_client_clock_receive_msg(data);
        if(resp != NULL){
            time = gst_clock_get_time(self->priv->internal_clock);
            gst_dvb_css_wc_client_clock_update(self, resp, time);
            msg_type = resp->message_type;
            g_free(resp);
        }

        if(msg_type == GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP){        
            resp = gst_dvb_css_wc_client_clock_receive_msg(data);
            if(resp != NULL){
                time = gst_clock_get_time(self->priv->internal_clock);
                gst_dvb_css_wc_client_clock_update(self, resp, time);
                g_free(resp);
            }
        }               
    }
  }
  GST_INFO_OBJECT (self, "shutting down dvb client clock thread");
  return NULL;  
}



static void
gst_dvb_css_wc_client_clock_synced_cb (GstClock * internal_clock, gboolean synced,
    GstClock * self)
{
  gst_clock_set_synced (self, synced);
}

static void
gst_dvb_css_wc_client_clock_constructed (GObject * object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  G_OBJECT_CLASS (gst_dvb_css_wc_client_clock_parent_class)->constructed (object);
  self->priv->internal_clock = gst_system_clock_obtain();
  
  GST_OBJECT_LOCK (self->priv->internal_clock);
  if (gst_clock_is_synced (self->priv->internal_clock))
    gst_clock_set_synced (GST_CLOCK (self), TRUE);
  self->priv->synced_id =
      g_signal_connect (self->priv->internal_clock, "synced",
      G_CALLBACK (gst_dvb_css_wc_client_clock_synced_cb), self);
  GST_OBJECT_UNLOCK (self->priv->internal_clock);
          
  if (!gst_dvb_css_wc_client_clock_start (self)) {
    g_warning ("failed to start clock '%s'", GST_OBJECT_NAME (self));
  }          
}

/**
 * gst_dvb_css_wc_client_clock_new:
 * @name: a name for the clock
 * @remote_address: the address or hostname of the remote clock provider
 * @remote_port: the port of the remote clock provider
 * @base_time: initial time of the clock
 *
 * Create a new #GstNetClientInternalClock that will report the time
 * provided by the #GstNetTimeProvider on @remote_address and 
 * @remote_port.
 *
 * Returns: a new #GstClock that receives a time from the remote
 * clock.
 */
GstClock *
gst_dvb_css_wc_client_clock_new (const gchar * name, const gchar * remote_address,
    gint remote_port, GstClockTime base_time)
{
  GstClock *ret;
  
  g_return_val_if_fail (remote_address != NULL, NULL);
  g_return_val_if_fail (remote_port > 0, NULL);
  g_return_val_if_fail (remote_port <= G_MAXUINT16, NULL);
  g_return_val_if_fail (base_time != GST_CLOCK_TIME_NONE, NULL);

  ret = g_object_new (GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, "address", remote_address,
      "port", remote_port, "base-time", base_time, NULL);

  return ret;
}

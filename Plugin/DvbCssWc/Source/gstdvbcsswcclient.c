/* GStreamer
 * Copyright (C) 2016
 *  Authors:  Wojciech Kapsa PSNC <kapsa@man.poznan.pl>
 *  Authors:  Xavi Artigas <xavi.artigas@i2cat.net>
 *  Authors:  Daniel Piesik <dpiesik@man.poznan.pl>
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

#include "gstdvbcsswcclient.h"
#include "gstdvbcsswcpacket.h"
#include "gstdvbcsswccommon.h"

#include <gio/gio.h>
#include <string.h>

#define DEFAULT_ADDRESS          "127.0.0.1"
#define DEFAULT_PORT             5637
#define DEFAULT_TIMEOUT          GST_SECOND
#define DEFAULT_BASE_TIME        0
#define DEFAULT_MAX_FREQ_ERR_PPM 500
#define DEFAULT_SOCKET_TIMEOUT   G_USEC_PER_SEC / 2

GST_DEBUG_CATEGORY_STATIC (dvbcss_wc_client);
#define GST_CAT_DEFAULT   (dvbcss_wc_client)
#define _do_init GST_DEBUG_CATEGORY_INIT (dvbcss_wc_client, "dvbcss_wc_client", 0, "DVB CSS WC client clock");

enum
{
  PROP_0,
  PROP_ADDRESS,
  PROP_PORT,
  PROP_BUS,
  PROP_BASE_TIME,
  PROP_INTERNAL_CLOCK,
};


//==============================================================================
// CANDIDATE
//==============================================================================

typedef struct candidate Candidate;

struct candidate
{
  GstClockTime      t1;
  GstClockTime      t2;
  GstClockTime      t3;
  GstClockTime      t4;
  GstClockTimeDiff  rtt;
  GstClockTimeDiff  offset;
  gdouble           precision_secs;
  guint32           max_freq_error_ppm;
  GstDvbCssWcPacket msg;
};

static Candidate*   candidate_new     (GstDvbCssWcPacket *msg);
static GstClockTime calc_dispersion   (GstClock *clock, gdouble local_precision_sec, guint32 local_max_freq_err_ppm, Candidate *candidate);

//==============================================================================
//==============================================================================

static Candidate*
candidate_new (GstDvbCssWcPacket *msg)
{
  Candidate* ret          = g_malloc (sizeof (Candidate));
  ret->t1                 = wc_timestamp_to_gst_clock_time(msg->originate_timevalue_secs, msg->originate_timevalue_nanos);
  ret->t2                 = msg->receive_timevalue;
  ret->t3                 = msg->transmit_timevalue;
  ret->t4                 = msg->response_timevalue;
  ret->rtt                = (GstClockTimeDiff)((ret->t4 - ret->t1) - (ret->t3 - ret->t2));
  ret->offset             = (GstClockTimeDiff)((ret->t3 + ret->t2) - (ret->t4 + ret->t1)) / 2;
  ret->precision_secs     = gst_dvb_css_wc_packet_decode_precision(msg->precision);
  ret->max_freq_error_ppm = (guint32)gst_dvb_css_wc_packet_decode_max_freq_error(msg->max_freq_error);
  return ret;
}

static GstClockTime
calc_dispersion (GstClock *clock, gdouble local_precision_sec, guint32 local_max_freq_err_ppm, Candidate *candidate)
{
  GstClockTime dispersion = (GstClockTime)(
                            (candidate->rtt / 2)                     +
                            (GST_SECOND * candidate->precision_secs) +
                            (GST_SECOND * local_precision_sec)       +
                            ((
                              (                                 local_max_freq_err_ppm  * (candidate->t4 - candidate->t1)                      ) +
                              (                          candidate->max_freq_error_ppm  * (candidate->t3 - candidate->t2)                      ) +
                              ((candidate->max_freq_error_ppm + local_max_freq_err_ppm) * (gst_clock_get_internal_time (clock) - candidate->t4))
                             ) / 1000000
                            ));
  return dispersion;
}

//==============================================================================
// GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK
//==============================================================================

#define GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK            (gst_dvb_css_wc_client_internal_clock_get_type())
#define GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK, GstDvbCssWcClientInternalClock))
#define GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK, GstDvbCssWcClientInternalClockClass))
#define GST_IS_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK))
#define GST_IS_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK))

G_GNUC_INTERNAL GType gst_dvb_css_wc_client_internal_clock_get_type(void);

typedef struct _GstDvbCssWcClientInternalClock      GstDvbCssWcClientInternalClock;
typedef struct _GstDvbCssWcClientInternalClockClass GstDvbCssWcClientInternalClockClass;

struct _GstDvbCssWcClientInternalClock
{
  GstSystemClock  clock;

  GThread        *thread;

  GSocket        *socket;
  GSocketAddress *servaddr;
  GCancellable   *cancel;
  gboolean        made_cancel_fd;

  gchar          *address;
  gint            port;

  /* Protected by OBJECT_LOCK */
  GList          *busses;

  //algorithm specific
  Candidate      *best_candidate;
  guint32         max_freq_error_ppm;
  gint64          socket_timeout;
  gdouble         clock_precision_sec;
};

struct _GstDvbCssWcClientInternalClockClass
{
  GstSystemClockClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GstDvbCssWcClientInternalClock, gst_dvb_css_wc_client_internal_clock, GST_TYPE_SYSTEM_CLOCK, _do_init);

static void               gst_dvb_css_wc_client_internal_clock_class_init   (GstDvbCssWcClientInternalClockClass *klass);
static void               gst_dvb_css_wc_client_internal_clock_init         (GstDvbCssWcClientInternalClock *self);

static void               gst_dvb_css_wc_client_internal_clock_finalize     (GObject *object);
static void               gst_dvb_css_wc_client_internal_clock_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void               gst_dvb_css_wc_client_internal_clock_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void               gst_dvb_css_wc_client_internal_clock_constructed  (GObject *object);

static gboolean           gst_dvb_css_wc_client_internal_clock_start        (GstDvbCssWcClientInternalClock *self);
static void               gst_dvb_css_wc_client_internal_clock_stop         (GstDvbCssWcClientInternalClock *self);
static gpointer           gst_dvb_css_wc_client_internal_clock_thread       (gpointer data);
static gboolean           gst_dvb_css_wc_client_internal_clock_send_request (gpointer data);
static GstDvbCssWcPacket* gst_dvb_css_wc_client_internal_clock_receive_msg  (gpointer data);
static void               gst_dvb_css_wc_client_internal_clock_update       (GstDvbCssWcClientInternalClock *self, GstDvbCssWcPacket *pkt);

//==============================================================================
//==============================================================================

static void
gst_dvb_css_wc_client_internal_clock_class_init (GstDvbCssWcClientInternalClockClass *klass)
{
  GObjectClass *gobject_class;
  gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = gst_dvb_css_wc_client_internal_clock_finalize;
  gobject_class->get_property = gst_dvb_css_wc_client_internal_clock_get_property;
  gobject_class->set_property = gst_dvb_css_wc_client_internal_clock_set_property;
  gobject_class->constructed  = gst_dvb_css_wc_client_internal_clock_constructed;

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
}

static void
gst_dvb_css_wc_client_internal_clock_init (GstDvbCssWcClientInternalClock *self)
{
  GST_OBJECT_FLAG_SET (self, GST_CLOCK_FLAG_NEEDS_STARTUP_SYNC);

  gst_clock_set_timeout (GST_CLOCK (self), DEFAULT_TIMEOUT);

  self->thread                    = NULL;
  self->servaddr                  = NULL;
  self->address                   = g_strdup (DEFAULT_ADDRESS);
  self->port                      = DEFAULT_PORT;
  self->best_candidate            = NULL;
  self->max_freq_error_ppm        = DEFAULT_MAX_FREQ_ERR_PPM;
  self->socket_timeout            = DEFAULT_SOCKET_TIMEOUT;
  self->clock_precision_sec       = measure_precision_sec (GST_CLOCK_CAST (self));
}

static void
gst_dvb_css_wc_client_internal_clock_finalize (GObject *object)
{
  GstDvbCssWcClientInternalClock *self = GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (object);

  if (self->thread)
  {
    gst_dvb_css_wc_client_internal_clock_stop (self);
  }

  g_free (self->address);
  self->address = NULL;

  if (self->servaddr != NULL)
  {
    g_object_unref (self->servaddr);
    self->servaddr = NULL;
  }

  if (self->socket != NULL)
  {
    if (!g_socket_close (self->socket, NULL))
    {
      GST_ERROR_OBJECT (self, "Failed to close socket");
    }
    g_object_unref (self->socket);
    self->socket = NULL;
  }

  g_warn_if_fail (self->busses == NULL);

  G_OBJECT_CLASS (gst_dvb_css_wc_client_internal_clock_parent_class)->finalize (object);
}

static void
gst_dvb_css_wc_client_internal_clock_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstDvbCssWcClientInternalClock *self = GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (object);

  switch (prop_id)
  {
    case PROP_ADDRESS:
    {
      GST_OBJECT_LOCK (self);
      g_free (self->address);
      self->address = g_value_dup_string (value);
      if (self->address == NULL)
      {
        self->address = g_strdup (DEFAULT_ADDRESS);
      }
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_PORT:
    {
      GST_OBJECT_LOCK (self);
      self->port = g_value_get_int (value);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static void
gst_dvb_css_wc_client_internal_clock_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstDvbCssWcClientInternalClock *self = GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (object);

  switch (prop_id)
  {
    case PROP_ADDRESS:
    {
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->address);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_PORT:
    {
      g_value_set_int (value, self->port);
      break;
    }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static void
gst_dvb_css_wc_client_internal_clock_constructed (GObject *object)
{
  GstDvbCssWcClientInternalClock *self = GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (object);

  G_OBJECT_CLASS (gst_dvb_css_wc_client_internal_clock_parent_class)->constructed (object);

  if (!gst_dvb_css_wc_client_internal_clock_start (self))
  {
    g_warning ("failed to start clock '%s'", GST_OBJECT_NAME (self));
  }
}

static gboolean
gst_dvb_css_wc_client_internal_clock_start (GstDvbCssWcClientInternalClock *self)
{
  GSocketAddress *servaddr;
  GSocketAddress *myaddr;
  GSocketAddress *anyaddr;
  GInetAddress   *inetaddr;
  GSocket        *socket;
  GError         *error = NULL;
  GSocketFamily   family;
  GPollFD         dummy_pollfd;
  GResolver      *resolver = NULL;
  GError         *err = NULL;

  g_return_val_if_fail (self->address != NULL, FALSE);
  g_return_val_if_fail (self->servaddr == NULL, FALSE);

  /* create target address */
  inetaddr = g_inet_address_new_from_string (self->address);
  if (inetaddr == NULL)
  {
    GList *results;
    resolver = g_resolver_get_default ();
    results  = g_resolver_lookup_by_name (resolver, self->address, NULL, &err);
    if (!results)
    {
      goto failed_to_resolve;
    }
    inetaddr = G_INET_ADDRESS (g_object_ref (results->data));
    g_resolver_free_addresses (results);
    g_object_unref (resolver);
  }

  family = g_inet_address_get_family (inetaddr);

  servaddr = g_inet_socket_address_new (inetaddr, self->port);
  g_object_unref (inetaddr);

  g_assert (servaddr != NULL);

  GST_INFO_OBJECT (self, "connecting to %s:%d", self->address, self->port);

  socket = g_socket_new (family, G_SOCKET_TYPE_DATAGRAM, G_SOCKET_PROTOCOL_UDP, &error);
  if (socket == NULL)
  {
    goto no_socket;
  }

  GST_TRACE_OBJECT (self, "binding socket");
  inetaddr = g_inet_address_new_any (family);
  anyaddr  = g_inet_socket_address_new (inetaddr, 0);
  g_socket_bind (socket, anyaddr, TRUE, &error);
  g_object_unref (anyaddr);
  g_object_unref (inetaddr);

  if (error != NULL)
  {
    goto bind_error;
  }

  /* check address we're bound to, mostly for debugging purposes */
  myaddr = g_socket_get_local_address (socket, &error);

  if (myaddr == NULL)
  {
    goto getsockname_error;
  }

  GST_DEBUG_OBJECT (self, "socket opened on UDP port %d", g_inet_socket_address_get_port (G_INET_SOCKET_ADDRESS (myaddr)));

  g_object_unref (myaddr);

  self->cancel = g_cancellable_new ();
  self->made_cancel_fd = g_cancellable_make_pollfd (self->cancel, &dummy_pollfd);

  self->socket = socket;
  self->servaddr = G_SOCKET_ADDRESS (servaddr);

  self->thread = g_thread_try_new ("GstDvbCssWcClientInternalClock", gst_dvb_css_wc_client_internal_clock_thread, self, &error);
  if (error != NULL)
  {
    goto no_thread;
  }

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
failed_to_resolve:
  {
    GST_ERROR_OBJECT (self, "resolving '%s' failed: %s",
        self->address, err->message);
    g_clear_error (&err);
    g_object_unref (resolver);
    return FALSE;
  }
no_thread:
  {
    GST_ERROR_OBJECT (self, "could not create thread: %s", error->message);
    g_object_unref (self->servaddr);
    self->servaddr = NULL;
    g_object_unref (self->socket);
    self->socket = NULL;
    g_error_free (error);
    return FALSE;
  }
}

static void
gst_dvb_css_wc_client_internal_clock_stop (GstDvbCssWcClientInternalClock *self)
{
  if (self->thread == NULL)
  {
    return;
  }

 GST_TRACE_OBJECT (self, "stopping...");
 g_cancellable_cancel (self->cancel);

 g_thread_join (self->thread);
 self->thread = NULL;

 if (self->made_cancel_fd)
 {
   g_cancellable_release_fd (self->cancel);
 }

 g_object_unref (self->cancel);
 self->cancel = NULL;

 g_object_unref (self->servaddr);
 self->servaddr = NULL;

 g_object_unref (self->socket);
 self->socket = NULL;

 GST_DEBUG_OBJECT (self, "stopped");
}

static gpointer
gst_dvb_css_wc_client_internal_clock_thread (gpointer data)
{
  GstDvbCssWcClientInternalClock *self          = data;
  GSocket                        *socket        = self->socket;
  gboolean                        success       = FALSE;
  guint8                          last_msg_type = GST_DVB_CSS_WC_MSG_REQUEST;
  GstDvbCssWcPacket              *resp          = NULL;
  GstClockTime                    response_time;

  GST_TRACE_OBJECT (self, "dvb client clock thread running, socket=%p", socket);

  g_socket_set_blocking (socket, TRUE);
  g_socket_set_timeout (socket, 0);

  while (!g_cancellable_is_cancelled (self->cancel))
  {
    success = gst_dvb_css_wc_client_internal_clock_send_request(data);
    if(success == TRUE)
    {
      while( (resp = gst_dvb_css_wc_client_internal_clock_receive_msg(data)) != NULL )
      {
        switch(resp->message_type)
        {
          case GST_DVB_CSS_WC_MSG_REQUEST:
          {
            continue;
            break;
          }
          case GST_DVB_CSS_WC_MSG_RESPONSE:
          {
            GST_LOG_OBJECT(self, "RESPONSE received on %" GST_TIME_FORMAT, GST_TIME_ARGS(resp->response_timevalue));
            gst_dvb_css_wc_client_internal_clock_update(self, resp);
          }
          case GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP:
          {
            GST_LOG_OBJECT(self, "RESPONSE received on %" GST_TIME_FORMAT ", wait for FOLLOWUP", GST_TIME_ARGS(resp->response_timevalue));
            response_time = resp->response_timevalue;
            break;
          }
          case GST_DVB_CSS_WC_MSG_FOLLOWUP:
          {
            if(last_msg_type == GST_DVB_CSS_WC_MSG_RESPONSE_WITH_FOLLOWUP)
            {
              // in followup message we use time of receiving the last message
              GST_LOG_OBJECT(self, "FOLLOWUP received, RESPONSE time is %" GST_TIME_FORMAT, GST_TIME_ARGS(response_time));
              resp->response_timevalue = response_time;
              gst_dvb_css_wc_client_internal_clock_update(self, resp);
            }
            break;
          }
          default:
          {
            GST_ERROR_OBJECT (self, "received unsupported message type");
          }
        }

        last_msg_type = resp->message_type;
        gst_dvb_css_wc_packet_free(resp);
      }
    }
  }
  GST_TRACE_OBJECT (self, "shutting down dvb client clock thread");
  return NULL;
}

static gboolean
gst_dvb_css_wc_client_internal_clock_send_request (gpointer data)
{
  GstDvbCssWcClientInternalClock *self    = data;
  GError                         *err     = NULL;
  GstDvbCssWcPacket              *req;
  GstClockTime                    time;
  gboolean                        success;

  req = gst_dvb_css_wc_packet_new (NULL);
  req->message_type = GST_DVB_CSS_WC_MSG_REQUEST;

  time = gst_clock_get_internal_time( GST_CLOCK_CAST (self));
  req->originate_timevalue_secs  = gst_clock_time_to_wc_timestamp_seconds(time);
  req->originate_timevalue_nanos = gst_clock_time_to_wc_timestamp_fraction(time);

  success = gst_dvb_css_wc_packet_send (req, self->socket, self->servaddr, &err);
  if (err != NULL)
  {
    GST_ERROR_OBJECT (self, "request send error: %s", err->message);
    g_error_free (err);
    err = NULL;
  }
  gst_dvb_css_wc_packet_free (req);

  return success;
}

static GstDvbCssWcPacket*
gst_dvb_css_wc_client_internal_clock_receive_msg(gpointer data)
{
  GstDvbCssWcClientInternalClock *self = data;
  GError                         *err  = NULL;
  GstDvbCssWcPacket              *resp;
  GstClockTime                    time;

  GST_TRACE_OBJECT (self, "set timeout: %" G_GINT64_FORMAT "us", self->socket_timeout);
  if (!g_socket_condition_timed_wait (self->socket, G_IO_IN, self->socket_timeout, self->cancel, &err))
  {
    /* cancelled, timeout or error */
    if (err->code == G_IO_ERROR_CANCELLED)
    {
      GST_TRACE_OBJECT (self, "cancelled %s", err->message);
      g_clear_error (&err);
      return NULL;
    }
    if(err->code == G_IO_ERROR_TIMED_OUT) GST_TRACE_OBJECT (self, "timeout: %s", err->message);
    else GST_WARNING_OBJECT (self, "socket error: %s", err->message);
    /* try again */
    g_usleep (G_USEC_PER_SEC / 10);
    g_error_free (err);
    err = NULL;
    return NULL;
  }

  /* got data in */
  time = gst_clock_get_internal_time( GST_CLOCK_CAST (self));
  resp = gst_dvb_css_wc_packet_receive (self->socket, &self->servaddr, &err);
  if (err != NULL)
  {
    GST_ERROR_OBJECT (self, "receive error: %s", err->message);
    g_usleep (G_USEC_PER_SEC / 10);
    g_error_free (err);
    err = NULL;
    return NULL;
  }

  resp->response_timevalue = time;
  return resp;
}

static void
gst_dvb_css_wc_client_internal_clock_update(GstDvbCssWcClientInternalClock *self, GstDvbCssWcPacket *pkt)
{
  Candidate    *candidate            = NULL;
  GstClockTime  current_dispersion   = G_MAXUINT64;
  GstClockTime  candidate_dispersion;
  GstClockTime  internal             = 0;
  GstClockTime  external             = 0;
  GstClockTime  rate_num             = 0;
  GstClockTime  rate_denom           = 0;

  if(pkt == NULL)
  {
    return;
  }

  if(pkt->message_type == GST_DVB_CSS_WC_MSG_REQUEST)
  {
    return;
  }

  if(self->best_candidate != NULL)
  {
    current_dispersion = calc_dispersion(GST_CLOCK_CAST (self), self->clock_precision_sec, self->max_freq_error_ppm, self->best_candidate);
  }

  candidate = candidate_new(pkt);
  if(candidate != NULL)
  {
    candidate_dispersion = calc_dispersion( GST_CLOCK_CAST (self), self->clock_precision_sec, self->max_freq_error_ppm, candidate);
    GST_LOG_OBJECT(self, "Current dispersion: %" G_GUINT64_FORMAT " \tCandidate dispersion: %" G_GUINT64_FORMAT "\n", current_dispersion, candidate_dispersion);
    if(current_dispersion >= candidate_dispersion)
    {
      GST_DEBUG_OBJECT(self, "Clock updated. Offset: %" G_GINT64_FORMAT "\n", candidate->offset);
      if(self->best_candidate != NULL)
      {
        g_free(self->best_candidate);
      }
      self->best_candidate = candidate;
      gst_clock_get_calibration (GST_CLOCK_CAST (self), &internal, &external, &rate_num, &rate_denom);
      gst_clock_set_calibration (GST_CLOCK_CAST (self), internal, candidate->offset, rate_num, rate_denom);
      gst_clock_set_synced( GST_CLOCK (self), TRUE);
    }
    else
    {
      g_free(candidate);
    }
  }
}

//==============================================================================
// GST_DVB_CSS_WC_CLIENT_CLOCK_PRIVATE
//==============================================================================

#define GST_DVB_CSS_WC_CLIENT_CLOCK_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, GstDvbCssWcClientClockPrivate))

struct _GstDvbCssWcClientClockPrivate
{
  GstClock     *internal_clock;

  GstClockTime  base_time;
  GstClockTime  internal_base_time;

  gchar        *address;
  gint          port;
  GstBus       *bus;
  gulong        synced_id;
};


//==============================================================================
// GST_DVB_CSS_WC_CLIENT_CACHE
//==============================================================================

typedef struct
{
  GstClock *clock;              /* GstDvbCssWcClientInternalClock */
  GList    *clocks;             /* GstDvbCssWcClientClocks */

  GstClockID remove_id;
} ClockCache;

G_LOCK_DEFINE_STATIC (clocks_lock);
static GList *clocks = NULL;

static void        finalize_clock_cache     (GObject *object);
static void        set_property_clock_cache (GObject *object);
static void        update_clock_cache       (ClockCache *cache);
static ClockCache* constructed_clock_cache  (GObject *object);
static gboolean    remove_clock_cache       (GstClock *clock, GstClockTime time, GstClockID id, gpointer user_data);

//==============================================================================
//==============================================================================

static void
finalize_clock_cache (GObject *object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  GList *l;

  G_LOCK (clocks_lock);
  for (l = clocks; l; l = l->next)
  {
    ClockCache *cache = l->data;
    if (cache->clock == self->priv->internal_clock)
    {
      cache->clocks = g_list_remove (cache->clocks, self);
      if (cache->clocks)
      {
        update_clock_cache (cache);
      }
      else
      {
        GstClock *sysclock = gst_system_clock_obtain ();
        GstClockTime time = gst_clock_get_time (sysclock) + 60 * GST_SECOND;
        cache->remove_id = gst_clock_new_single_shot_id (sysclock, time);
        gst_clock_id_wait_async (cache->remove_id, remove_clock_cache, cache, NULL);
        gst_object_unref (sysclock);
      }
      break;
    }
  }
  G_UNLOCK (clocks_lock);
}

static void
set_property_clock_cache (GObject *object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  GList *l;

  G_LOCK (clocks_lock);
  for (l = clocks; l; l = l->next)
  {
    ClockCache *cache = l->data;
    if (cache->clock == self->priv->internal_clock)
    {
      update_clock_cache (cache);
    }
  }
  G_UNLOCK (clocks_lock);
}

/* Must be called with clocks_lock */
static void
update_clock_cache (ClockCache * cache)
{
  GList *l      = NULL;
  GList *busses = NULL;

  GST_OBJECT_LOCK (cache->clock);

  g_list_free_full (GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (cache->clock)->busses, (GDestroyNotify) gst_object_unref);
  for (l = cache->clocks; l; l = l->next)
  {
    GstDvbCssWcClientClock *clock = l->data;

    if (clock->priv->bus)
    {
      busses = g_list_prepend (busses, gst_object_ref (clock->priv->bus));
    }
  }
  GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (cache->clock)->busses = busses;

  GST_OBJECT_UNLOCK (cache->clock);
}

static ClockCache*
constructed_clock_cache (GObject *object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  ClockCache *cache = NULL;
  GList *l = NULL;

  G_LOCK (clocks_lock);
  for (l = clocks; l; l = l->next)
  {
    ClockCache *tmp = l->data;
    GstDvbCssWcClientInternalClock *internal_clock = GST_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK (tmp->clock);

    if (strcmp (internal_clock->address, self->priv->address) == 0 && internal_clock->port == self->priv->port)
    {
      cache = tmp;
      if (cache->remove_id)
      {
        gst_clock_id_unschedule (cache->remove_id);
        cache->remove_id = NULL;
      }
      break;
    }
  }

  if (!cache)
  {
    cache = g_new0 (ClockCache, 1);
    cache->clock = g_object_new (GST_TYPE_DVB_CSS_WC_CLIENT_INTERNAL_CLOCK, "address", self->priv->address, "port", self->priv->port, NULL);
    clocks = g_list_prepend (clocks, cache);

    /* Not actually leaked but is cached for a while before being disposed,
     * see gst_net_client_clock_finalize, so pretend it is to not confuse
     * tests. */
    //GST_OBJECT_FLAG_SET (cache->clock, GST_OBJECT_FLAG_MAY_BE_LEAKED);
  }

  cache->clocks = g_list_prepend (cache->clocks, self);

  G_UNLOCK (clocks_lock);

  return cache;
}

static gboolean
remove_clock_cache (GstClock *clock, GstClockTime time, GstClockID id, gpointer user_data)
{
  ClockCache *cache = user_data;

  G_LOCK (clocks_lock);
  if (!cache->clocks)
  {
    gst_clock_id_unref (cache->remove_id);
    gst_object_unref (cache->clock);
    clocks = g_list_remove (clocks, cache);
    g_free (cache);
  }
  G_UNLOCK (clocks_lock);

  return TRUE;
}


//==============================================================================
// GST_DVB_CSS_WC_CLIENT_CLOCK
//==============================================================================

G_DEFINE_TYPE (GstDvbCssWcClientClock, gst_dvb_css_wc_client_clock, GST_TYPE_SYSTEM_CLOCK);

static void         gst_dvb_css_wc_client_clock_class_init        (GstDvbCssWcClientClockClass *klass);
static void         gst_dvb_css_wc_client_clock_init              (GstDvbCssWcClientClock *self);

static void         gst_dvb_css_wc_client_clock_finalize          (GObject *object);
static void         gst_dvb_css_wc_client_clock_set_property      (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void         gst_dvb_css_wc_client_clock_get_property      (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static void         gst_dvb_css_wc_client_clock_constructed       (GObject *object);
static void         gst_dvb_css_wc_client_clock_synced_cb         (GstClock *internal_clock, gboolean synced, GstClock *self);
static GstClockTime gst_dvb_css_wc_client_clock_get_internal_time (GstClock *clock);

//==============================================================================
//==============================================================================

static void
gst_dvb_css_wc_client_clock_class_init (GstDvbCssWcClientClockClass *klass)
{
  GObjectClass  *gobject_class;
  GstClockClass *clock_class;

  gobject_class = G_OBJECT_CLASS  (klass);
  clock_class   = GST_CLOCK_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GstDvbCssWcClientClockPrivate));

  gobject_class->finalize     = gst_dvb_css_wc_client_clock_finalize;
  gobject_class->get_property = gst_dvb_css_wc_client_clock_get_property;
  gobject_class->set_property = gst_dvb_css_wc_client_clock_set_property;
  gobject_class->constructed  = gst_dvb_css_wc_client_clock_constructed;

  clock_class->get_internal_time = gst_dvb_css_wc_client_clock_get_internal_time;

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
}

static void
gst_dvb_css_wc_client_clock_init (GstDvbCssWcClientClock *self)
{
  GST_OBJECT_FLAG_SET (self, GST_CLOCK_FLAG_CAN_SET_MASTER);
  GST_OBJECT_FLAG_SET (self, GST_CLOCK_FLAG_NEEDS_STARTUP_SYNC);

  GstClock *clock;
  clock = gst_system_clock_obtain ();

  GstDvbCssWcClientClockPrivate *priv;
  self->priv = priv = GST_DVB_CSS_WC_CLIENT_CLOCK_GET_PRIVATE (self);

  priv->port                    = DEFAULT_PORT;
  priv->address                 = g_strdup (DEFAULT_ADDRESS);
  priv->base_time               = DEFAULT_BASE_TIME;
  priv->internal_base_time      = gst_clock_get_time (clock);

  gst_object_unref (clock);
}

static void
gst_dvb_css_wc_client_clock_finalize (GObject *object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);

  finalize_clock_cache(object);

  if (self->priv->synced_id)
  {
    g_signal_handler_disconnect (self->priv->internal_clock, self->priv->synced_id);
  }
  self->priv->synced_id = 0;

  g_free (self->priv->address);
  self->priv->address = NULL;

  if (self->priv->bus != NULL)
  {
    gst_object_unref (self->priv->bus);
    self->priv->bus = NULL;
  }

  G_OBJECT_CLASS (gst_dvb_css_wc_client_clock_parent_class)->finalize (object);
}

static void
gst_dvb_css_wc_client_clock_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  gboolean update = FALSE;

  switch (prop_id)
  {
    case PROP_ADDRESS:
    {
      GST_OBJECT_LOCK (self);
      g_free (self->priv->address);
      self->priv->address = g_value_dup_string (value);
      if (self->priv->address == NULL)
      {
        self->priv->address = g_strdup (DEFAULT_ADDRESS);
      }
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_PORT:
    {
      GST_OBJECT_LOCK (self);
      self->priv->port = g_value_get_int (value);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_BUS:
    {
      GST_OBJECT_LOCK (self);
      if (self->priv->bus)
      {
        gst_object_unref (self->priv->bus);
      }
      self->priv->bus = g_value_dup_object (value);
      GST_OBJECT_UNLOCK (self);
      update = TRUE;
      break;
    }
    case PROP_BASE_TIME:
    {
      GstClock *clock;
      self->priv->base_time = g_value_get_uint64 (value);
      clock = gst_system_clock_obtain ();
      self->priv->internal_base_time = gst_clock_get_time (clock);
      gst_object_unref (clock);
      break;
    }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }

  if (update && self->priv->internal_clock)
  {
    set_property_clock_cache (object);
  }
}

static void
gst_dvb_css_wc_client_clock_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);

  switch (prop_id)
  {
    case PROP_ADDRESS:
    {
      GST_OBJECT_LOCK (self);
      g_value_set_string (value, self->priv->address);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_PORT:
    {
      g_value_set_int (value, self->priv->port);
      break;
    }
    case PROP_BUS:
    {
      GST_OBJECT_LOCK (self);
      g_value_set_object (value, self->priv->bus);
      GST_OBJECT_UNLOCK (self);
      break;
    }
    case PROP_BASE_TIME:
    {
      g_value_set_uint64 (value, self->priv->base_time);
      break;
    }
    case PROP_INTERNAL_CLOCK:
    {
      g_value_set_object (value, self->priv->internal_clock);
      break;
    }
    default:
    {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
  }
}

static void
gst_dvb_css_wc_client_clock_constructed (GObject *object)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (object);
  GstClock *internal_clock;
  ClockCache *cache = NULL;

  G_OBJECT_CLASS (gst_dvb_css_wc_client_clock_parent_class)->constructed (object);

  cache = constructed_clock_cache(object);

  GST_OBJECT_LOCK (cache->clock);
  if (gst_clock_is_synced (cache->clock))
  {
    gst_clock_set_synced (GST_CLOCK (self), TRUE);
  }
  self->priv->synced_id = g_signal_connect (cache->clock, "synced", G_CALLBACK (gst_dvb_css_wc_client_clock_synced_cb), self);
  GST_OBJECT_UNLOCK (cache->clock);

  self->priv->internal_clock = internal_clock = cache->clock;
}

static void
gst_dvb_css_wc_client_clock_synced_cb (GstClock *internal_clock, gboolean synced, GstClock *self)
{
  gst_clock_set_synced (self, synced);
}

static GstClockTime
gst_dvb_css_wc_client_clock_get_internal_time (GstClock *clock)
{
  GstDvbCssWcClientClock *self = GST_DVB_CSS_WC_CLIENT_CLOCK (clock);

  if (!gst_clock_is_synced (self->priv->internal_clock))
  {
    GstClockTime now = gst_clock_get_internal_time (self->priv->internal_clock);
    return gst_clock_adjust_with_calibration (self->priv->internal_clock, now, self->priv->internal_base_time, self->priv->base_time, 1, 1);
  }
  return gst_clock_get_time (self->priv->internal_clock);
}


//==============================================================================
// GST_DVB_CSS_WC_CLIENT_CLOCK
//==============================================================================

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
GstClock*
gst_dvb_css_wc_client_clock_new (const gchar *name, const gchar *remote_address, gint remote_port, GstClockTime base_time)
{
  GstClock *ret;

  g_return_val_if_fail (remote_address != NULL, NULL);
  g_return_val_if_fail (remote_port > 0, NULL);
  g_return_val_if_fail (remote_port <= G_MAXUINT16, NULL);
  g_return_val_if_fail (base_time != GST_CLOCK_TIME_NONE, NULL);

  ret = g_object_new (GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, "address", remote_address, "port", remote_port, "base-time", base_time, NULL);
  return ret;
}

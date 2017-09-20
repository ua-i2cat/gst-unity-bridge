#ifndef __DVB_CSS_WC_CLIENT_H__
#define __DVB_CSS_WC_CLIENT_H__

/* GStreamer
 * Copyright (C) 2016 
 *  Authors:  Wojciech Kapsa PSNC <kapsa@man.poznan.pl>
 *  Authors:  Xavi Artigas <xavi.artigas@i2cat.net>
 *  Authors:  Daniel Piesik <dpiesik@man.poznan.pl>
 *
 * gstdvbcsswcclient.h: clock that synchronizes itself to a time over
 * the network using DVB CSS WC
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

#include <gst/gst.h>
#include <gst/gstsystemclock.h>

G_BEGIN_DECLS

#define GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK            (gst_dvb_css_wc_client_clock_get_type())
#define GST_DVB_CSS_WC_CLIENT_CLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, GstDvbCssWcClientClock))
#define GST_DVB_CSS_WC_CLIENT_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK, GstDvbCssWcClientClockClass))
#define GST_IS_DVB_CSS_WC_CLIENT_CLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK))
#define GST_IS_DVB_CSS_WC_CLIENT_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GST_TYPE_DVB_CSS_WC_CLIENT_CLOCK))

typedef struct _GstDvbCssWcClientClock        GstDvbCssWcClientClock;
typedef struct _GstDvbCssWcClientClockClass   GstDvbCssWcClientClockClass;
typedef struct _GstDvbCssWcClientClockPrivate GstDvbCssWcClientClockPrivate;

/**
 * GstDvbCssWcClient:
 *
 * Opaque #GstDvbCssWcClient structure.
 */
struct _GstDvbCssWcClientClock
{
  GstSystemClock clock;

  /*< private >*/
  GstDvbCssWcClientClockPrivate *priv;
  gpointer _gst_reserved[GST_PADDING];
};

struct _GstDvbCssWcClientClockClass
{
  GstSystemClockClass parent_class;

  /*< private >*/
  gpointer _gst_reserved[GST_PADDING];
};

/**
 * gst_dvb_css_wc_client_clock_new:
 * @name: a name for the clock
 * @remote_address: the address or hostname of the remote clock provider
 * @remote_port: the port of the remote clock provider
 * @base_time: initial time of the clock
 *
 * Create a new #GstClock that will report the time
 * provided by the #GstDvbCssWcServer on @remote_address and 
 * @remote_port.
 *
 * Returns: a new #GstClock that receives a time from the remote
 * clock.
 */
GstClock*	gst_dvb_css_wc_client_clock_new      (const gchar *name, const gchar *remote_address, gint remote_port, GstClockTime base_time);
GType     gst_dvb_css_wc_client_clock_get_type (void);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstDvbCssWcClientClock, gst_object_unref)
#endif

G_END_DECLS

#endif /* __DVB_CSS_WC_CLIENT_H__ */

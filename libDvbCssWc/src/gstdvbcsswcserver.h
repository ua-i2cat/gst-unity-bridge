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


#ifndef __GST_DVB_CSS_WC_SERVER_H__
#define __GST_DVB_CSS_WC_SERVER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_DVB_CSS_WC_SERVER \
  (gst_dvb_css_wc_server_get_type())
#define GST_DVB_CSS_WC_SERVER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_DVB_CSS_WC_SERVER,GstDvbCssWcServer))
#define GST_DVB_CSS_WC_SERVER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_DVB_CSS_WC_SERVER,GstDvbCssWcServerClass))
#define GST_IS_DVB_CSS_WC_SERVER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_DVB_CSS_WC_SERVER))
#define GST_IS_DVB_CSS_WC_SERVER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_DVB_CSS_WC_SERVER))

typedef struct _GstDvbCssWcServer GstDvbCssWcServer;
typedef struct _GstDvbCssWcServerClass GstDvbCssWcServerClass;
typedef struct _GstDvbCssWcServerPrivate GstDvbCssWcServerPrivate;

/**
 * GstDvbCssWcServer:
 *
 * Opaque #GstDvbCssWcServer structure.
 */
struct _GstDvbCssWcServer {
  GstObject parent;

  /*< private >*/
  GstDvbCssWcServerPrivate *priv;

  gpointer _gst_reserved[GST_PADDING];
};

struct _GstDvbCssWcServerClass {
  GstObjectClass parent_class;

  gpointer _gst_reserved[GST_PADDING];
};

GType                   gst_dvb_css_wc_server_get_type  (void);

/**
 * gst_dvb_css_wc_server_new:
 * @clock: a #GstClock to export over the network
 * @address: (allow-none): an address to bind on as a dotted quad
 *           (xxx.xxx.xxx.xxx), IPv6 address, or NULL to bind to all addresses
 * @port: a port to bind on, or 0 to let the kernel choose
 *
 * Allows network clients to get the current time of @clock.
 *
 * Returns: the new #GstDvbCssWcServer, or NULL on error
 */
GstDvbCssWcServer*     gst_dvb_css_wc_server_new       (GstClock *clock,
                                                         const gchar *address,
                                                         gint port,
                                                         gboolean followup,
                                                         guint32 max_freq_error_ppm);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC(GstDvbCssWcServer, gst_object_unref)
#endif

G_END_DECLS


#endif /* __GST_DVB_CSS_WC_SERVER_H__ */

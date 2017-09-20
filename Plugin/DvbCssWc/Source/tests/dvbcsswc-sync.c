// #include <iostream>
// #include <string>
// #include <stdio.h>
// #include <cmath>

#include <gst/net/gstnet.h>
#include <gstdvbcsswcclient.h>

static const char* SERVER_ADDRESS          = "192.168.53.190";
static const int         SERVER_GSTNET_PORT      = 5001;
static const int         SERVER_GSTDVBCSSWC_PORT = 5000;
static const int         MAX_SYNC_WAIT_TIME_SEC  = 5;

static gboolean synchronization(GstClock *inClock);

int main (int argc, char *argv[])
{
  gst_init(&argc, &argv);

  GstClock *net_clock = gst_net_client_clock_new       ("net_clock", (gchar*)SERVER_ADDRESS, SERVER_GSTNET_PORT,      0);
  GstClock *dvb_clock = gst_dvb_css_wc_client_clock_new("dvb_clock", (gchar*)SERVER_ADDRESS, SERVER_GSTDVBCSSWC_PORT, 0);

  synchronization(net_clock);
  synchronization(dvb_clock);

  unsigned int counter = 0;
  while(TRUE)
  {
    GstClockTime net_time = gst_clock_get_time(net_clock);
    GstClockTime dvb_time = gst_clock_get_time(dvb_clock);

    gboolean net_is_synced = gst_clock_is_synced(net_clock);
    gboolean dvb_is_synced = gst_clock_is_synced(dvb_clock);

    g_print("%03d: \tnet = %015" G_GUINT64_FORMAT " (%d) \tdvb = %015" G_GUINT64_FORMAT " (%d) \n"
            , counter, net_time, net_is_synced, dvb_time, dvb_is_synced
          );

    ++counter;
    g_usleep(G_USEC_PER_SEC);
  }

  gst_object_unref(net_clock);
  gst_object_unref(dvb_clock);

  return 0;
}


static gboolean synchronization(GstClock *inClock)
{
  gst_clock_wait_for_sync(inClock, MAX_SYNC_WAIT_TIME_SEC * GST_SECOND);
  gboolean synced = gst_clock_is_synced(inClock);
  if(synced == FALSE)
  {
    g_print("Could not synced clock\n");
  }
  return synced;
}

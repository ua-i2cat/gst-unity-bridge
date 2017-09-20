#include "../export/client-export.h"

int main(int argc, char *argv[])
{
  GstClock* dvb_client = gst_dvb_css_wc_client_start("dvbClient", "192.168.53.190", 5000, 0);

  int counter = 0;
  while(counter < 10)
  {
    GstClockTime time      = gst_dvb_css_wc_client_get_time(dvb_client);
    gboolean     is_synced = gst_dvb_css_wc_client_is_synced(dvb_client);
    g_print("%03d: time = %015" G_GUINT64_FORMAT " \tis_synced = %d \n", counter, time, is_synced);
    g_usleep(G_USEC_PER_SEC);
    ++counter;
  }

  gst_dvb_css_wc_client_stop(dvb_client);

  return 0;
}

#ifndef __DVB_CSS_WC_COMMON_H__
#define __DVB_CSS_WC_COMMON_H__

#include <gst/gst.h>

#define PRECISION_MEASUR_NUMBER  100

static gdouble measure_precision_sec  (GstClock *clock);

//==============================================================================
//==============================================================================

static gdouble
measure_precision_sec (GstClock *clock)
{
  if(clock == NULL)
  {
    return 0;
  }

  GstClockTimeDiff diff_max = 0;

  int i;
  for(i = 0; i < PRECISION_MEASUR_NUMBER; ++i)
  {
    GstClockTime     measurement_1 = gst_clock_get_internal_time (clock);
    GstClockTime     measurement_2 = gst_clock_get_internal_time (clock);
    GstClockTimeDiff diff          = measurement_2 - measurement_1;
    if(diff > diff_max)
    {
      diff_max = diff;
    }
  }
  gdouble precision_sec = GST_TIME_AS_SECONDS( gst_guint64_to_gdouble(diff_max));
  return precision_sec;
}

#endif /* __DVB_CSS_WC_COMMON_H__ */

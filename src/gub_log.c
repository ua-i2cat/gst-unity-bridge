/*
 * GStreamer - Unity3D bridge.
 * (C) 2015 i2CAT
 */

#include "gub.h"
#include <stdio.h>
#include <stdarg.h>

#if !defined(__ANDROID__)

void gub_log(const char *format, ...)
{
	static FILE *logf = NULL;
	va_list args;

	if (!logf) {
		logf = fopen("gub.log", "wt");
	}
	va_start(args, format);
	vfprintf(logf, format, args);
	fprintf(logf, "\n");
	va_end(args);
}

#else

#include <android/log.h>

void gub_log(const char *format, ...)
{
	va_list args;
	gchar *msg;
	va_start(args, format);
	msg = g_strdup_vprintf(format, args);
	__android_log_print(ANDROID_LOG_INFO, "gub", msg);
	g_free(msg);
	va_end(args);
}

#endif

/*
*  GStreamer - Unity3D bridge (GUB).
*  Copyright (C) 2016  Fundacio i2CAT, Internet i Innovacio digital a Catalunya
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Lesser General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*  Authors:  Xavi Artigas <xavi.artigas@i2cat.net>
*/

#include "gub_log.h"
#include <stdio.h>
#include <stdarg.h>

GUBUnityDebugLogPFN gub_unity_debug_log = NULL;

EXPORT_API void gub_log_set_unity_handler(GUBUnityDebugLogPFN pfn) {
    gub_unity_debug_log = pfn;
}

#if !defined(__ANDROID__)

void gub_log(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    if (!gub_unity_debug_log) {
        static FILE *logf = NULL;
        if (!logf) {
            logf = fopen("gub.log", "w+t");
        }
        vfprintf(logf, format, args);
        fprintf(logf, "\n");
        fflush(logf);
    } else {
        gchar *message = g_strdup_vprintf(format, args);
        gub_unity_debug_log(3, message);
        g_free(message);
    }
    va_end(args);
}

#else

#include <android/log.h>
#include <gst/gst.h>

void gub_log(const char *format, ...)
{
    va_list args;
    gchar *msg;
    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    __android_log_print(ANDROID_LOG_INFO, "gub", "%s", msg);
    g_free(msg);
    va_end(args);
}

#endif

void gub_log_error(const char *message)
{
    if (gub_unity_debug_log) {
        gub_unity_debug_log(0, message);
    }
    else {
        gub_log("ERROR: %s", message);
    }
}

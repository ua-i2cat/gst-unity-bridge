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
    fflush(logf);
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

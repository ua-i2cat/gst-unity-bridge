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

#include <gst/gst.h>
#include <stdio.h>
#include "gub.h"

EXPORT_API void gub_ref(const char *gst_debug_string);

EXPORT_API void gub_unref();

EXPORT_API gint32 gub_is_active();

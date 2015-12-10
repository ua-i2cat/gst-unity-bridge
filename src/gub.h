/*
* GStreamer - Unity3D bridge.
* Based on https://github.com/mrayy/mrayGStreamerUnity
*/

#ifndef __GUB_H__
#define __GUB_H__

#define EXPORT_API __declspec(dllexport)

void gub_copy_texture(const char *data, int w, int h, void *_TextureNativePtr);

#endif
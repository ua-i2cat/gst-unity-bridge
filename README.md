# GStreamer - Unity3D Bridge

Put GStreamer pipelines inside Unity3D

Inspired on code from https://github.com/mrayy/mrayGStreamerUnity

The pipeline description must have an element named `sink` which must have a property named `last-frame`.
When Unity asks for a texture update, the last frame is taken from that element and passed onto Unity.
`fakesink` works perfectly well.

Tested on Linux, Windows and Android. Synchronized reasonabily well over Ethernet and Wifi (LAN).

## Building on Android

Take this into account: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055

Then, the usual:

```
android update project -p . -s --target android-23

ndk-build
```

No need to go further in the build process, the library is already available in the `libs` folder.

## Network clock synchronization

If enabled, will try to synchronize to a network clock.

Use this on the server side, for example (from gst-rtsp-server):

```
./test-netclock "( filesrc location=~/sintel-1024-surround.mp4 ! qtdemux name=d ! queue ! rtph264pay name=pay0 pt=96 d. ! queue ! rtpmp4apay name=pay1 pt=96 )"
```

And this on the pipeline description inside Unity:

```
uridecodebin uri="rtsp://127.0.0.1:8554/test" name=s ! queue ! videoconvert ! video/x-raw,format=RGB ! fakesink sync=1 name=sink s. ! queue ! audioconvert ! audioresample ! autoaudiosink
```

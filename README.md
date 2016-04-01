# GStreamer - Unity3D Bridge (GUB)

Play any media URI inside [Unity3D](http://www.unity3d.com) textures using [GStreamer 1.x](http://gstreamer.freedesktop.org) pipelines.

Inspired on code from https://github.com/mrayy/mrayGStreamerUnity

Tested on Windows and Android.

The system is composed of a bunch of C# scripts to be used inside Unity, which interact with a native plugin (`GstUnityBridge.dll` or `libGstUnityBridge.so`).

The plugin, in turn, calls the GStreamer libraries, which must be available on the system.

On Android, GStreamer is statically linked into a single library which can be deployed with your application (`libGstUnityBridge.so` and `libgstreamer_android.so`).

A sample project, unimaginatively called `GstUnityBridgeTest1` is included in the `tests` folder.
It contains prebuilt binaries for **Windows** (x86 and x86_64) and **Android** (arm).

## 1. Usage

The easiest path is probably to copy the contents of the `Assets\Plugins` and `Assets\Scripts` folders from the sample project
into your Unity project's `Assets` folder. Just make sure that each native plugin (`.dll` and `.so`) is assigned by Unity to its proper platform.
Then, from within the Unity editor, drag the `GstUnityBridgeTexture` script onto your object (the other scripts cannot be used as Components).

A number of properties are available:

  - **FlipX**: Invert texture horizontally
  - **FlipY**: Invert texture vertically
  - **URI**: The URI of the media stream. GStreamer understands several protocols, for example, rtsp:// or http://.
  - **Video Index**: Which stream to use when the input URI contains more than one video stream. This is a zero-based index. -1 disables video altogether.
  - **Audio Index**: Which stream to use when the input URI contains more than one audio stream. This is a zero-based index. -1 disables audio altogether.
  - **Initialize On Start**: Whether the GStreamer pipeline must be created and set to PLAYING when the scene is loaded. Leave this on unless you plan to activate the pipeline manually from a script.

More properties are available, detailed in the next sections:

### Network synchronization

When enabled, will synchronize the GStreamer pipeline to a GStreamer network clock.
This allows synchronized playback across multiple devices. The clock server address and port must be provided through properties:

  - **Enable**: Enable network clock usage. If unchecked, the following two properties are ignored.
  - **Master Clock Address**: Server address (IP or host name) of the clock provider.
  - **Master Clock Port**: Port where the clock is provided.

The easiest way to provide a network stream **and** and clock is through the [gst-rtsp-server](http://cgit.freedesktop.org/gstreamer/gst-rtsp-server/) example `test-netclock`.
For example, use this on the server:

```
./test-netclock "( filesrc location=~/sintel-1024-surround.mp4 ! qtdemux name=d ! queue ! rtph264pay name=pay0 pt=96 d. ! queue ! rtpmp4apay name=pay1 pt=96 )"
```

This will create an RTSP stream at `rtsp://127.0.0.1:8554/test`. Then add these properties to your object inside Unity:

  - **URI**: `rtsp://YOUR.SERVER.ADDRESS:8554/test`
  - **Clock Address**: `YOUR.SERVER.ADDRESS`
  - **Clock Port**: `8554`
  - **Video Index**: 0
  - **Audio Index**: 1

### Video Cropping

The incoming video can be cropped before using in Unity.
This can be very handy, but keep in mind that the whole video is transmitted and decoded, which might be wasteful if the amount of cropping is large.

  - **Left**, **Top**, **Right**, **Bottom**: Amount to crop from each edge, from 0.0 (no crop) to 1.0 (full crop, no image is displayed)

## 2. Building the native plugin
It is easier to use the prebuilt binaries included in the test project. However, should you need to build your own native plugin, use these instructions.

### Building the plugin for Windows
Solution and Project files are included for Visual Studio 2015 (works with the Free Community Edition). Just open and build.
Make sure the latest [GStreamer 1.x SDK](http://gstreamer.freedesktop.org/data/pkg/windows/) is installed and the
`GSTREAMER_1_0_ROOT_X86` or `GSTREAMER_1_0_ROOT_X86_64` environment variable is defined and points to the proper place.
Copy the resulting `GstUnityBridge.dll` to the `Assets\Plugins\x86` or `x86_64` folder of your Unity project.

### Building the plugin for Android
First off, some patches for GStreamer are needed (`$GST_PREFIX` is the folder where you installed GStreamer, for example, `C:\gstreamer\1.0\android-arm`):

- Take this into account if using a GStreamer version below 1.6.2: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055
- In `$GST_PREFIX/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h`, make sure `GST_GL_HAVE_GLSYNC` is defined to 1

Then, the usual:

```
android update project -p . -s --target android-23
ndk-build
```

At this point, two libraries are already available in the `libs` folder: `libgstreamer_android.so` and `libGstUnityBridge.so`.
Both must be copied to the `Assets\Plugins\android_arm` folder of your Unity project.

Then you need to compile the Java part. The easiest way is probably to build the whole project and then generate the JAR file with the needed classes:

```
ant release
cd bin/classes
jar cvf gub.jar org/*
```

And copy the resulting `gub.jar` file to the `Assets\Plugins\android_arm` folder of your Unity project.

### Building the plugin for Linux
No facilities are given yet (no Makefiles), but this has worked in the past:

```
gcc -shared -fPIC -Wl,--no-as-needed `pkg-config --cflags --libs gstreamer-1.0 gstreamer-net-1.0 gstreamer-video-1.0` *.c -o libGstUnityBridge.so
```

And then copy the resulting `libGstUnityBridge.so` to the `Assets\Plugins\Linux` folder of your Unity project.

## 3. TODO

- Better error reporting (when sync fails, for example)
- Allow using other synchronization mechanisms (NTP or PTP, for example)
- Due to some unknown issue with the Android GStreamer audio sink, presence breaks network synchronization.
- The Unity3D Editor loads all native plugins at startup, so it does not pick up changes you make later on. https://github.com/mrayy/mrayGStreamerUnity already took care of this.
- iOS support
- OSX support
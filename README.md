# GStreamer - Unity3D Bridge

Put [GStreamer 1.0](http://gstreamer.freedesktop.org) pipelines inside [Unity3D](http://www.unity3d.com).

Inspired on code from https://github.com/mrayy/mrayGStreamerUnity

A flexible approach has been followed, so the plugin is both lightweight and generic.

Tested on Windows and Android.

The system is composed of a bunch of C# scripts to be used inside Unity, which interact with a native plugin (`.dll` or `.so`).
The plugin, in turn, calls the GStreamer libraries, which must be available on the system.
On Android, GStreamer is linked statically to the plugin.

A sample project, unimaginatively called `GstUnityBridgeTest1` is included in the `tests` folder.
It contains prebuilt binaries for **Windows** (x86 and x86_64) and **Android** (arm).

## 1. Usage

The easiest path is probably to copy the contents of the `Assets\Plugins` and `Assets\Scripts` folders from the sample project
into your Unity project's `Assets` folder. Just make sure that each native plugin (`.dll` and `.so`) is assigned by Unity to its proper platform.
Then, from within the Unity editor, drag the `GstUnityBridgeTexture` script onto your object (disregard the other scripts).

A number of properties are available:

  - **FlipX**: Invert texture horizontally
  - **FlipY**: Invert texture vertically
  - **URI**: The URI of the media stream. GStreamer understands several protocols, for example, rtsp:// or http://.
  - **Video Index**: Which stream to use when the input URI contains more than one video stream. This is a zero-based index. -1 disables video altogether.
  - **Audio Index**: Which stream to use when the input URI contains more than one audio stream. This is a zero-based index. -1 disables audio altogether.
  - **Initialize On Start**: Whether the GStreamer pipeline must be created and set to PLAYING when the scene is loaded. Leave this on unless you plan to activate the pipeline manually.

More properties are available, related to network sycnhronization, detailed below.

### Network clock synchronization

When enabled, will synchronize the GStreamer pipeline to a GStreamer network clock.
This allows synchronized playback across multiple devices. The clock server address and port must be provided through properties:

  - **Use Network Synchronization**: Enable network clock usage. If unchecked, the following two properties are ignored.
  - **Clock Address**: Server address (IP or host name) of the clock provider.
  - **Clock Port**: Port where the clock is provided.

The easiest way to provide a network stream **and** and clock is through the [gst-rtsp-server](http://cgit.freedesktop.org/gstreamer/gst-rtsp-server/) example `test-netclock`.
For example, use this on the server:

```
./test-netclock "( filesrc location=~/sintel-1024-surround.mp4 ! qtdemux name=d ! queue ! rtph264pay name=pay0 pt=96 d. ! queue ! rtpmp4apay name=pay1 pt=96 )"
```

This will create an RTSP stream at `rtsp://127.0.0.1:8554/test`. Then add these properties to your object inside Unity:

  - **URI**: `rtsp://YOUR.SERVER.ADDRESS:8554/test`

  - **Clock Address**: `YOUR.SERVER.ADDRESS`

  - **Clock Port**: `8554`

## 2. Building the native plugin
It is easier to use the prebuilt binaries included in the test project. However, should you need to build your own native plugin, use these instructions.

### Building the plugin for Windows
Solution and Project files are included for Visual Studio 2015 (works with the Free Community Edition). Just open and build.
Make sure the latest [GStreamer 1.0 SDK](http://gstreamer.freedesktop.org/data/pkg/windows/) is installed and the
`GSTREAMER_1_0_ROOT_X86` or `GSTREAMER_1_0_ROOT_X86_64` environment variable is defined and points to the proper place.
Copy the resulting `GstUnityBridge.dll` to the `Assets\Plugins` folder of your Unity project.

### Building the plugin for Android
First off, some patches for GStreamer (`$GST_PREFIX` is the folder where you installed GStreamer, for example, `C:\gstreamer\1.0\android-arm`):

- Take this into account if using a GStreamer version below 1.6.2: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055
- In `$GST_PREFIX/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h`, make sure `GST_GL_HAVE_GLSYNC` is defined to 1
- In `$GST_PREFIX/share/gst-android/ndk-build/gstreamer_android-1.0.c.in`, #if 0 all methods after `gst_android_load_gio_modules`, and the three variables at the top (`_context`, `_class_loader` and `_priv_gst_info_start_time`)

Then, the usual:

```
android update project -p . -s --target android-23
ndk-build
```

No need to go further in the build process, two libraries are already available in the `libs` folder: `libgstreamer_android.so` and `libGstUnityBridge.so`.
Both must be copied to the `Assets\Plugins` folder of your Unity project.

### Building the plugin for Linux
No facilities are given yet (no Makefiles), but this has worked in the past:

```
gcc -shared -fPIC -Wl,--no-as-needed `pkg-config --cflags --libs gstreamer-1.0 gstreamer-net-1.0 gstreamer-video-1.0` *.c -o libGstUnityBridge.so
```

## 3. Known issues

- Due to some unknown issue with the Android GStreamer audio sink, its presence breaks network synchronization.
- The Unity3D Editor loads all native plugins at startup, so it does not pick up changes you make later on. https://github.com/mrayy/mrayGStreamerUnity already took care of this.

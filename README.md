# GStreamer - Unity3D Bridge (GUB)

Play any media URI inside [Unity3D](http://www.unity3d.com) textures using [GStreamer 1.x](http://gstreamer.freedesktop.org) pipelines.

Inspired on code from https://github.com/mrayy/mrayGStreamerUnity

Tested on Windows and Android.

The system is composed of a bunch of C# scripts to be used inside Unity, which interact with a native plugin (`GstUnityBridge.dll` or `libGstUnityBridge.so`).

The plugin, in turn, calls the GStreamer libraries, which can be deployed alongside the plugin, or it will use the system's ones.

On Android, GStreamer is statically linked into a single library which can be deployed with your application (`libGstUnityBridge.so` and `libgstreamer_android.so`).

The `Plugin` folder contains the source code for building the native plugin, with its own readme.txt file.

The `Unity` folder contains the prebuilt plugins, C# scripts and sample scenes, along with its own readme.txt file.

DVB-CSS-WC support.
1. Compile libDvbCssWc/win32/DvbCssWc.sln using Visual Studio for x86.
2. Compile GstUnityBridge.sln using Visual Studio for x86.
3. Copy libDvbCssWc.dll into the same place as GstUnityBridge.dll

# TODO

- Better error reporting (when sync fails, for example)
- Allow using other synchronization mechanisms (NTP or PTP, for example)
- Due to some unknown issue with the Android GStreamer audio sink, presence breaks network synchronization.
- The Unity3D Editor loads all native plugins at startup, so it does not pick up changes you make later on. https://github.com/mrayy/mrayGStreamerUnity already took care of this.
- iOS support
- OSX support

# REQUIREMENT
- Unity version 5.6.0 or higuer

# ACKNOWLEDGEMENT

This software has been created within the [ImmersiaTV](http://immersiatv.eu) project. This project has received funding from the European Union�s Horizon 2020 research and innovation programme under grant agreement 688619.

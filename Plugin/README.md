#Building the native plugin
It is easier to use the prebuilt binaries included in the test project. However, should you need to build your own native plugin, use these instructions.

Building the plugin for Windows
---
Solution and Project files are included for Visual Studio 2015 (works with the Free Community Edition). Just open and build.
Make sure the latest GStreamer 1.x is installed (http://gstreamer.freedesktop.org/data/pkg/windows/) and the
GSTREAMER_1_0_ROOT_X86 or GSTREAMER_1_0_ROOT_X86_64 environment variable is defined and points to the proper place.
The project already copies the resulting GstUnityBridge.dll to the Unity\Assets\Plugins\x86 or x86_64 folder of the included Unity project.

Building the plugin for Android
---
Follow the instructions allocated in GUB/Project/Android.

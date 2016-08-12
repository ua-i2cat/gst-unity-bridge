Building the native plugin

It is easier to use the prebuilt binaries included in the test project. However, should you need to build your own native plugin, use these instructions.

1. Building the plugin for Windows

Solution and Project files are included for Visual Studio 2015 (works with the Free Community Edition). Just open and build.
Make sure the latest GStreamer 1.x is installed (http://gstreamer.freedesktop.org/data/pkg/windows/) and the
GSTREAMER_1_0_ROOT_X86 or GSTREAMER_1_0_ROOT_X86_64 environment variable is defined and points to the proper place.
The project already copies the resulting GstUnityBridge.dll to the Unity\Assets\Plugins\x86 or x86_64 folder of the included Unity project.

2. Building the plugin for Android

-Download Android SDK (Linux version) command line tools from https://developer.android.com/studio/index.html
-Add SDK tools dir to PATH
-Call "android update sdk --no-ui"

-Download Android NDK (Linux) command line tools from https://developer.android.com/ndk/downloads/index.html
-Add NDK root dir to PATH

-Download Gstreamer Android (for selected platform arm, arm64 etc.) tar from https://gstreamer.freedesktop.org/data/pkg/android/
-Add GSTREAMER_ROOT_ANDROID variable and point it to the extracted tar root
-Go to unzipped dir and found: ./lib/gstreamer-1.0/include/gst/gl/gstglconfig.h, set:
#define GST_GL_HAVE_GLSYNC 1
#define GST_GL_HAVE_GLUINT64 1
#define GST_GL_HAVE_GLINT64 1

-Add some patches if using a GStreamer version below 1.6.2: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055

Then, the usual:

WARN: android-ndk-r12 have a bug with undefined reference to 'bsd_signal', ndk-r13 will fix it.

on android-ndk-r12:
android update project -p . -s --target android-19
in this case libgstreamer_android.so don't work on API level higher than 19 (e.g. android 6). Use libgstreamer_android.so compiled on higher API level with current libGstUnityBridge.so.

on android-ndk-r13:
android update project -p . -s --target android-23

ndk-build -k 

At this point, two libraries are already available in the libs folder: libgstreamer_android.so and libGstUnityBridge.so
Both must be copied to the Assets\Plugins\android_arm folder of your Unity project.

The first time, you will need to compile the Java part. The easiest way is probably to build the whole project and then generate the JAR file with the needed classes:

ant release
cd bin/classes
jar cvf gub.jar org/*

And copy the resulting gub.jar file to the Assets\Plugins\android_arm folder of your Unity project.

3. Building the plugin for Linux

No facilities are given yet (no Makefiles), but this has worked in the past:

gcc -shared -fPIC -Wl,--no-as-needed `pkg-config --cflags --libs gstreamer-1.0 gstreamer-net-1.0 gstreamer-video-1.0` *.c -o libGstUnityBridge.so

And then copy the resulting libGstUnityBridge.so to the Assets\Plugins\Linux folder of your Unity project.

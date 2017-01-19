Building the native plugin

It is easier to use the prebuilt binaries included in the test project. However, should you need to build your own native plugin, use these instructions.

1. Building the plugin for Windows

Solution and Project files are included for Visual Studio 2015 (works with the Free Community Edition). Just open and build.
Make sure the latest GStreamer 1.x is installed (http://gstreamer.freedesktop.org/data/pkg/windows/) and the
GSTREAMER_1_0_ROOT_X86 or GSTREAMER_1_0_ROOT_X86_64 environment variable is defined and points to the proper place.
The project already copies the resulting GstUnityBridge.dll to the Unity\Assets\Plugins\x86 or x86_64 folder of the included Unity project.

2. Building the plugin for Android

First off, some patches for GStreamer are needed ($GST_PREFIX is the folder where you installed GStreamer, for example, `C:\gstreamer\1.0\android-arm`):

- Take this into account if using a GStreamer version below 1.6.2: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055
- In $GST_PREFIX/lib/gstreamer-1.0/include/gst/gl/gstglconfig.h, make sure GST_GL_HAVE_GLSYNC is defined to 1

Then, the usual:

Download the android SDK 23, and tools 4.9
Latest archictecture for Gstreamer is fine, copy the gstglconfig.h from the libs directory into the gst/gl directory. This is probably an environment issue on my setup but it helps..
Set the -fuse-ld=gold to -fuse-ld=gold.exe as clang version 3.8 and newer need the EXE at the end

//Add the architecture to the Android.mk file - TARGET_ARCH_ABI := armeabi-v7a
//create an Application.mk under jini with the following lines:
//APP_ABI := armeabi-v7a # ideally, this should be set to "all"
//APP_PLATFORM := android-23 # should the same as -platform and your minSdkVersion.
Set all three of these to 1
#define GST_GL_HAVE_GLSYNC 1
#define GST_GL_HAVE_GLUINT64 1
#define GST_GL_HAVE_GLINT64 1

Make sure that the NDK paths are setup correctly as environment paths in windows.

android update project -p . -s --target android-23
ndk-build

At this point, two libraries are already available in the libs folder: libgstreamer_android.so and libGstUnityBridge.so
Both must be copied to the Assets\Plugins\android_arm folder of your Unity project.

The first time, you will need to compile the Java part. The easiest way is probably to build the whole project and then generate the JAR file with the needed classes:

install apache-ant (that is actually the java helper by apache)
set the apache-ant path (you could do this in the windows environment variables but I just did this because I'm lazy) - doskey ant=C:\Programming\apache-ant-1.10.0\bin\ant.bat $* 


ant release
cd bin/classes
jar cvf gub.jar org/*

And copy the resulting gub.jar file to the Assets\Plugins\android_arm folder of your Unity project.

3. Building the plugin for Linux

No facilities are given yet (no Makefiles), but this has worked in the past:

gcc -shared -fPIC -Wl,--no-as-needed `pkg-config --cflags --libs gstreamer-1.0 gstreamer-net-1.0 gstreamer-video-1.0` *.c -o libGstUnityBridge.so

And then copy the resulting libGstUnityBridge.so to the Assets\Plugins\Linux folder of your Unity project.

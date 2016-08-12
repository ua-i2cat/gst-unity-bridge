# GStreamer - DVB CSS WC plugin

Gstreamer Clock implementing the ETSI TS 103 286-2 V1.1.1 (2015-05) Wall Clock.

Tested on Windows 8.1, 10 and Linux Ubuntu 16.04 using Gstreamer 1.8.1

# Building

On Linux:
-On Ubuntu 16.04 call make
-On older distros install gstreamer 1.8.1 and missing dependencies

On Windows:
-For x86 download gstreamer-1.0-devel-x86-1.8.1.msi and gstreamer-1.0-x86-1.8.1.msi from https://gstreamer.freedesktop.org/download/ and install it.
-For x64 download gstreamer-1.0-devel-x86_64-1.8.1.msi and gstreamer-1.0-x86_64-1.8.1.msi from https://gstreamer.freedesktop.org/download/ and install it.
-Make sure the GSTREAMER_1_0_ROOT_X86 or GSTREAMER_1_0_ROOT_X86_64 environment variable is defined and points to the proper place.
-Open: win32/*sln in Visual Studio 2015
-Compile in VS for selected platform

# Running
Make sure that at runtime, GStreamer can access its libraries and plugins. It can be done by adding %GSTREAMER_SDK_ROOT_X86%\bin to the %PATH% environment variable,
or by running the application from this same folder.

List of dependecies on Windows:
-libglib-2.0-0.dll
-libgobject-2.0-0.dll
-libgstreamer-1.0-0.dll
-libwinpthread-1.dll

# TODO

Black box tests

# ACKNOWLEDGEMENT

This software has been created within the [ImmersiaTV](http://immersiatv.eu) project. This project has received funding from the European Union Horizon 2020 research and innovation programme under grant agreement 688619.

# GStreamer - Unity3D Bridge

Put GStreamer pipelines inside Unity3D

Inspired on code from https://github.com/mrayy/mrayGStreamerUnity

## Building on Android

Take this into account: https://bug757732.bugzilla-attachments.gnome.org/attachment.cgi?id=315055

Then, the usual:
```
android update project -p . -s --target android-23
ndk-build
```
No need to go further in the build process, the library is already available in the `libs` folder.

GStreamer Movie Texture

This asset allows playing media (Audio and Video) as a texture on Unity,
using the GStreamer framework (https://gstreamer.freedesktop.org), which
is contained in the Plugins folder.
Works for Windows (32 and 64 bits) and Android (arm_v7).

Works from within the Unity Editor on Windows, if it is launched in DirectX9
mode (Launch with the -force-d3d9 option).

Sample scenes are included.

USAGE
-----

Drag the GstUnityBridgeTexture script onto your object (the other scripts
cannot be used as Components), or instantiate from script.

PROPERTIES
----------

- FlipX: Invert texture horizontally
- FlipY: Invert texture vertically
- Loop: Play media from the beginning when it reaches the end
- URI: The URI of the media stream. GStreamer understands several protocols,
    including local and remote media. For example, file://, rtsp:// or http://
- Video Index: Which stream to use when the input URI contains more than one
    video stream. This is a zero-based index. -1 disables video altogether.
- Audio Index: Which stream to use when the input URI contains more than one
    audio stream. This is a zero-based index. -1 disables audio altogether.
- Initialize On Start: Whether the GStreamer pipeline must be created and set
    to PLAYING when the scene is loaded. Leave this on unless you plan to
    activate the pipeline manually from a script later on by calling
    Initialize(), Setup() and Play().
- Position: Is the current media position, in seconds. Can be read and written.
    Only accessible through scripting.
- Duration: Is the current media length, in seconds. Can only be read.
    Only accessible through scripting.
    The value is only correct if IsPlaying() returns true.

More properties are available, detailed in the next sections:

EVENTS

- OnFinish: Called when the media ends.
- OnError (string message): Called when there is any kind of error.

VIDEO CROPPING

The incoming video can be cropped before using in Unity. This can be very handy,
but keep in mind that the whole video is transmitted and decoded, which might be
wasteful if the amount of cropping is large.

- Left, Top, Right, Bottom: Amount to crop from each edge, from 0.0 (no crop) to
    1.0 (full crop, no image is displayed)

NETWORK SYNCHRONIZATION (Advanced usage)

When enabled, will synchronize the GStreamer pipeline to a GStreamer network
clock. This allows synchronized playback across multiple devices. The clock
server address and port must be provided through properties:

- Enable: Enable network clock usage. If unchecked, the following two
    properties are ignored.
- Master Clock Address: Server address (IP or host name) of the clock provider.
- Master Clock Port: Port where the clock is provided.

The quickest way to have a clock server running is using the test-netclock
example from https://cgit.freedesktop.org/gstreamer/gst-rtsp-server/

DEBUG OUTPUT

- Enabled: Enable to write debug output to the Unity Editor Console, LogCat on
    Android or gub.txt on a Standalone player.
- GStreamerDebugString: Comma-separated list of categories and log levels as
    used with the GST_DEBUG environment variable. Setting to '2' is normally
    enough. Leave empty to disable GStreamer debug output.

METHODS
-------

A number of methods ara available through scripting:

- Initialize():
    Call this before anything else.
    Called from Start() if InitializeOnStart=true.
- Setup(string URI, int videoIndec, int audioIndex):
    Set the URI and streams to play. Can be called multiple times.
    Called from Start() if InitializeOnStart=true.
- Play():
- Pause():
- Stop():
    No description. You have to guess.
    Play() is called automatically from Start() if InitializeOnStart=true.
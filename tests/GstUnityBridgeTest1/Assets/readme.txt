GStreamer Movie Texture

This asset allows playing media (Audio and Video) as a texture on Unity,
using the GStreamer framework (https://gstreamer.freedesktop.org/), which
is contained in the Plugins folder.
Works for Windows (32 and 64 bits) and Android (arm_v7).

Sample scenes are included.

USAGE

Drag the GstUnityBridgeTexture script onto your object (the other scripts
cannot be used as Components).

A number of properties are available:

- FlipX: Invert texture horizontally
- FlipY: Invert texture vertically
- URI: The URI of the media stream. GStreamer understands several protocols,
    including local and remote media. For example, file://, rtsp:// or http://
- Video Index: Which stream to use when the input URI contains more than one
    video stream. This is a zero-based index. -1 disables video altogether.
- Audio Index: Which stream to use when the input URI contains more than one
    audio stream. This is a zero-based index. -1 disables audio altogether.
- Initialize On Start: Whether the GStreamer pipeline must be created and set
    to PLAYING when the scene is loaded. Leave this on unless you plan to
	activate the pipeline manually from a script.

More properties are available, detailed in the next sections:

VIDEO CROPPING

The incoming video can be cropped before using in Unity. This can be very handy,
but keep in mind that the whole video is transmitted and decoded, which might be
wasteful if the amount of cropping is large.

Properties:
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

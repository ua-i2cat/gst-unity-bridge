#GStreamer Movie Texture

This asset allows playing media (Audio and Video) as a texture on Unity, using the GStreamer framework (https://gstreamer.freedesktop.org), which is contained in the Plugins folder.

Works for Windows (32 and 64 bits) and Android (arm_v7).

Works from within the Unity Editor on Windows, if it is launched in DirectX9 mode (Launch with the -force-d3d9 option).

Sample scenes are included.


##USAGE

1st) Drag GstUnityBridgeStarter script onto any gameobject of the scene. This scripts initializes and loads GStreamer. 
2nd) Drag the GstUnityBridgeTexture script onto your object with a Renderer component (ie: MeshRender), or instantiate from script.

##PROPERTIES

- FlipX: Invert texture horizontally
- FlipY: Invert texture vertically
- Loop: Play media from the beginning when it reaches the end
- URI: The URI of the media stream. GStreamer understands several protocols, including local and remote media. For example, file://, rtsp:// or http://
- Video Index: Which stream to use when the input URI contains more than one video stream. This is a zero-based index. -1 disables video altogether.
- Audio Index: Which stream to use when the input URI contains more than one audio stream. This is a zero-based index. -1 disables audio altogether.
- Target Material: Optional material whose texture will be replaced. If it is None, the first material in the Renderer of this GameObject will be used.
- Initialize On Start: Whether the GStreamer pipeline must be created and set to PLAYING when the scene is loaded. Leave this on unless you plan to activate the pipeline manually from a script later on by calling Initialize(), Setup() and Play().
- Position: Is the current media position, in seconds. Can be read and written. Only accessible through scripting.
- Duration: Is the current media length, in seconds. Can only be read. Only accessible through scripting. The value is only correct if IsPlaying() returns true.

More properties are available, detailed in the next sections:

###EVENTS

- OnFinish: Called when the media ends.
- OnError (string message): Called when there is any kind of error.
- OnQOS (QosData data): Called when frames are discarded due to lateness. The QosData structure contains information about the discarded frame and statistics regarding the whole playback session:
   - current_jitter: Nanoseconds. How late a buffer was. Negative is early, positive is late. Early buffers are not reported.
   - current_running_time: Nanoseconds. Running time when the late buffer was reported.
   - current_stream_time: Nanoseconds. Stream time when the late buffer was reported.
   - current_timestamp: Nanoseconds. Timestamp of the late buffer.
   - proportion: Long term prediction of the ideal rate relative to normal rate to get optimal quality.
   - processed: Total number of correctly processed frames.
   - dropped: Total number of dropped frames, because they were late.

Fore more information:
https://gstreamer.freedesktop.org/data/doc/gstreamer/head/pwg/html/chapter-advanced-qos.html
https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstMessage.html#gst-message-parse-qos

###VIDEO CROPPING

The incoming video can be cropped before using in Unity. This can be very handy, but keep in mind that the whole video is transmitted and decoded, which might be wasteful if the amount of cropping is large.

- Left, Top, Right, Bottom: Amount to crop from each edge, from 0.0 (no crop) to 1.0 (full crop, no image is displayed)

###NETWORK SYNCHRONIZATION (Advanced usage)

When enabled, will synchronize the GStreamer pipeline to a GStreamer network clock. This allows synchronized playback across multiple devices. The clock server address and port must be provided through properties:
- Enable: Enable network clock usage. If unchecked, the following two properties are ignored.
- Master Clock Address: Server address (IP or host name) of the clock provider.
- Master Clock Port: Port where the clock is provided.

The quickest way to have a clock server running is using the test-netclock example from:
https://cgit.freedesktop.org/gstreamer/gst-rtsp-server/

###DEBUG OUTPUT

- Enabled: Enable to write debug output to the Unity Editor Console, LogCat on Android or gub.txt on a Standalone player. Does not work properly when there is more than one instance of the plugin with different Debug settings.
- GStreamerDebugString: Comma-separated list of categories and log levels as used with the GST_DEBUG environment variable. Setting to '2' is normally enough. Leave empty to disable GStreamer debug output.


##METHODS

A number of methods ara available through scripting:

- Initialize():
    - Call this before anything else.
    - Called from Start() if InitializeOnStart=true.
- Setup(string URI, int videoIndec, int audioIndex):
    - Set the URI and streams to play. Can be called multiple times.
    - Called from Start() if InitializeOnStart=true.
- Play():
- Pause():
- Stop():
    - No description. You have to guess.
    Play() is called automatically from Start() if InitializeOnStart=true.

##ANDROID NOTES

The version of GStreamer does not support OpenGLES3 yet, therefore, Unity must be forced to use OpenGLES2 instead. To do so, go to:
- Edit -> Project Settings -> Player -> Android -> Other Settings -> Rendering
- And set "Auto Graphics API" to OFF.
- Then, in the list of graphic APIs, remove OpenGLES3 and leave only OpenGLES2.

Gub needs internet access, so it have to be activated:
- Edit -> Project Settings -> Player -> Android -> Other Settings -> Configuration
- And select "Require" in Internet Access options.

In Android builds Unity's default material doesn't work correctly with GUB. You should select/drag any of the materials from GstUnityBridge\Materials to the MeshRenderer component. We recommend "ExternalAlphaBase".


##SOURCE CODE

The project is Open Source. Code available at:
https://github.com/ua-i2cat/gst-unity-bridge

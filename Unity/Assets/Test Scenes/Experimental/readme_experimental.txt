EXPERIMENTAL FEATURES
---------------------

The scenes contained in this folder demonstrate some features of the GStreamer
Movie Texture which are still marked as EXPERIMENTAL, meaning that they might
be unstable, deprecated in the future or their usage might change. In any case,
they should not be relied upon.
To enable them, write EXPERIMENTAL in the Script Define Symbol box in the
Player Settings (Under Other Settings).

These features are:

PROPERTIES
----------
- Is Alpha: When this flag is not checked (normal use case) the video frames
    are used as the main texture (_MainTex) of the first material in the
    Renderer component of the Game Object. When the flag is set, instead, the
    video frames are used in a texture called _AlphaTex. When used with the
    provided ExternalAlpha shader, this allows having two GUBTexture components
    stacked over the same Game Object, one providing the video color and the
    other the video transparency.
- Base Time: Basetime allows connecting to an already playing pipeline, by
    using the same basetime on all players. Set to 0 to start playing the media
    from the beginning (normal use).

COMPONENTS
----------
- GstUnityBridgeCapture: On its Update() function will read the texture
    indicated in the Source property, encode it and write it into an MP4/H.264
    file.

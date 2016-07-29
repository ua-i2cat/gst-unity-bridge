LOCAL_PATH := $(call my-dir)
DVB_WC_PATH = $(LOCAL_PATH)/../../libDvbCssWc/

include $(CLEAR_VARS)

LOCAL_MODULE    := GstUnityBridge
LOCAL_C_INCLUDES := $(DVB_WC_PATH)/src/
LOCAL_SRC_FILES := ../src/gub_graphics.c ../src/gub_gstreamer.c ../src/gub_pipeline.c ../src/gub_log.c ../../libDvbCssWc/src/gstdvbcsswcclient.c ../../libDvbCssWc/src/gstdvbcsswcpacket.c
LOCAL_SHARED_LIBRARIES := gstreamer_android
LOCAL_LDLIBS := -llog -landroid -lGLESv2
include $(BUILD_SHARED_LIBRARY)

ifndef GSTREAMER_ROOT
ifndef GSTREAMER_ROOT_ANDROID
$(error GSTREAMER_ROOT_ANDROID is not defined!)
endif
GSTREAMER_ROOT        := $(GSTREAMER_ROOT_ANDROID)
endif
GSTREAMER_NDK_BUILD_PATH  := $(GSTREAMER_ROOT)/share/gst-android/ndk-build/
include $(GSTREAMER_NDK_BUILD_PATH)/plugins.mk
GSTREAMER_PLUGINS         := $(GSTREAMER_PLUGINS_CORE) $(GSTREAMER_PLUGINS_PLAYBACK) $(GSTREAMER_PLUGINS_CODECS) $(GSTREAMER_PLUGINS_NET) $(GSTREAMER_PLUGINS_SYS) $(GSTREAMER_PLUGINS_EFFECTS) $(GSTREAMER_PLUGINS_CAPTURE) $(GSTREAMER_PLUGINS_ENCODING) $(GSTREAMER_PLUGINS_CODECS_RESTRICTED)
G_IO_MODULES              := gnutls
GSTREAMER_EXTRA_DEPS      := gstreamer-video-1.0 gstreamer-net-1.0
include $(GSTREAMER_NDK_BUILD_PATH)/gstreamer-1.0.mk

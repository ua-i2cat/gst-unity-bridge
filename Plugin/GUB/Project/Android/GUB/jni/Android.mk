LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

IMMERSIATV_PLAYER_ROOT  := $(abspath $(LOCAL_PATH)/../../../../../..)
GUB_SOURCE_PATH         := $(IMMERSIATV_PLAYER_ROOT)/Plugin/GUB/Source
DVB_CSS_WC_HEADERS_PATH := $(IMMERSIATV_PLAYER_ROOT)/Plugin/DvbCssWc/Source
DVB_CSS_WC_PROJECT_PATH := $(IMMERSIATV_PLAYER_ROOT)/Plugin/DvbCssWc/Project/Android

LOCAL_MODULE            := GstUnityBridge
LOCAL_C_INCLUDES        := $(DVB_CSS_WC_HEADERS_PATH)
LOCAL_SRC_FILES         := $(GUB_SOURCE_PATH)/gub_graphics.c \
                           $(GUB_SOURCE_PATH)/gub_gstreamer.c \
                           $(GUB_SOURCE_PATH)/gub_pipeline.c \
                           $(GUB_SOURCE_PATH)/gub_log.c
LOCAL_SHARED_LIBRARIES  := gstreamer_android DvbCssWc
LOCAL_LDLIBS            := -llog -lGLESv2
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(DVB_CSS_WC_PROJECT_PATH)/jni/Android.mk

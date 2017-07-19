LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

GUB_ROOT := $(abspath $(LOCAL_PATH)/../../../../..)
DVB_CSS_WC_SOURCE_PATH := $(GUB_ROOT)/Plugin/DvbCssWc/Source
GSTREAMER_PROJECT_PATH := $(GUB_ROOT)/Plugin/Externals/gstreamer/Project/Android

LOCAL_MODULE           := DvbCssWc
LOCAL_SRC_FILES        := $(DVB_CSS_WC_SOURCE_PATH)/gstdvbcsswcclient.c \
                          $(DVB_CSS_WC_SOURCE_PATH)/gstdvbcsswcpacket.c \
                          $(DVB_CSS_WC_SOURCE_PATH)/gstdvbcsswcserver.c \
                          $(DVB_CSS_WC_SOURCE_PATH)/export/client-export.c \
                          $(DVB_CSS_WC_SOURCE_PATH)/export/server-export.c
LOCAL_SHARED_LIBRARIES := gstreamer_android
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(GSTREAMER_PROJECT_PATH)/jni/Android.mk

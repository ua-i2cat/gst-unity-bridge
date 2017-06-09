LOCAL_PATH := $(call my-dir)

GUB_PROJECT_PATH     := $(LOCAL_PATH)/../../GUB
GUB_SOURCE_PATH      := $(LOCAL_PATH)/../../../../Source
GUB_TEST_SOURCE_PATH := $(LOCAL_PATH)/../../../../Source/tests/android-test

include $(CLEAR_VARS)
LOCAL_MODULE      := testGUB-Android
LOCAL_C_INCLUDES  := $(GUB_SOURCE_PATH)
LOCAL_SRC_FILES   := ../$(GUB_TEST_SOURCE_PATH)/jni/renderer_wrapper.c \
                     ../$(GUB_TEST_SOURCE_PATH)/jni/testGUB-Android.c
LOCAL_SHARED_LIBRARIES := gstreamer_android GstUnityBridge
LOCAL_LDLIBS := -llog -landroid -lGLESv2
GUB_JAVA_SRC_DIR := src
$(shell mkdir -p $(GUB_JAVA_SRC_DIR))
$(shell cp -r $(GUB_TEST_SOURCE_PATH)/src/* $(GUB_JAVA_SRC_DIR))
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
include $(GUB_PROJECT_PATH)/jni/Android.mk

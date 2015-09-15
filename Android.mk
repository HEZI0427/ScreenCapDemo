LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# for executable file
#LOCAL_SRC_FILES:= \
#        main.cpp \
#    screencap.cpp \
#    openimage.cpp \
#    bmp.c

# for shared library
LOCAL_SRC_FILES:= \
    screencap.cpp \
    bmp.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libbinder \
    libskia \
    libui \
    libgui \
        libjpeg \
        libavcodec-2.1.4 \
        libavformat-2.1.4 \
        libavfilter-2.1.4 \
        libavresample-2.1.4 \
        libswresample-2.1.4 \
        libavdevice-2.1.4 \
        libavutil-2.1.4 \
        libswscale-2.1.4 \

#for executable file
#LOCAL_MODULE:= screencap

#for shared library
LOCAL_MODULE:= libscreencap

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
    system/core/include/android \
    external/skia/include/core \
    external/skia/include/effects \
    external/skia/include/images \
    external/skia/src/ports \
    external/skia/include/utils \
    external/jpeg \
        external/ffmpeg-2.1.4.android

LOCAL_CFLAGS += -DLOG_TAG=\"SCREENCAP\"
LOCAL_CFLAGS += -Wall -g -D__STDC_CONSTANT_MACROS
LOCAL_CPPFLAGS += -Wall -g

LOCAL_LDLIBS += -llog

#for shared library
include $(BUILD_SHARED_LIBRARY)
#for executable file
#include $(BUILD_EXECUTABLE)

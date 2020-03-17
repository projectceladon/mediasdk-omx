LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_OMX_HOME)/mfx_omx_defs.mk

LOCAL_SRC_FILES := \
    $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp))) \
    $(addprefix src/components/, $(notdir $(wildcard $(LOCAL_PATH)/src/components/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_OMX_INCLUDES) \
    $(MFX_OMX_INCLUDES_LIBVA) \
    $(MFX_OMX_HOME)/omx_utils/include \
    $(MFX_OMX_HOME)/omx_buffers/include

LOCAL_CFLAGS := \
    $(MFX_OMX_CFLAGS) \
    $(MFX_OMX_CFLAGS_LIBVA)

LOCAL_LDFLAGS := \
    $(MFX_OMX_LDFLAGS) \
    -Wl,--version-script=$(LOCAL_PATH)/omx_components.map

LOCAL_SHARED_LIBRARIES := \
    libdl liblog \
    libva libva-android \
    libhardware \
    libcutils \
    libui \
    libutils

LOCAL_SHARED_LIBRARIES_32 := libmfxhw32
LOCAL_SHARED_LIBRARIES_64 := libmfxhw64

LOCAL_STATIC_LIBRARIES := libmfx_omx_buffers libmfx_omx_utils
LOCAL_HEADER_LIBRARIES := $(MFX_OMX_HEADER_LIBRARIES)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfx_omx_components_hw

include $(BUILD_SHARED_LIBRARY)

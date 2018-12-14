LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
include $(MFX_OMX_HOME)/mfx_omx_defs.mk

LOCAL_SRC_FILES := $(addprefix src/, $(notdir $(wildcard $(LOCAL_PATH)/src/*.cpp)))

LOCAL_C_INCLUDES := \
    $(MFX_OMX_INCLUDES) \
    $(MFX_OMX_INCLUDES_LIBVA) \
    $(MFX_OMX_HOME)/omx_utils/include \
    $(MFX_OMX_HOME)/omx_utils/include/spl

LOCAL_CFLAGS := \
    $(MFX_OMX_CFLAGS) \
    $(MFX_OMX_CFLAGS_LIBVA)

LOCAL_HEADER_LIBRARIES := $(MFX_OMX_HEADER_LIBRARIES)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmfx_omx_buffers

include $(BUILD_STATIC_LIBRARY)

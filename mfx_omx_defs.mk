# Purpose:
#   Defines include paths, compilation flags, etc. to build Media SDK targets.
#
# Defined variables:
#   MFX_OMX_CFLAGS - common flags for all targets
#   MFX_OMX_CFLAGS_LIBVA - LibVA support flags (to build apps with or without LibVA support)
#   MFX_OMX_INCLUDES - common include paths for all targets
#   MFX_OMX_INCLUDES_LIBVA - include paths to LibVA headers
#   MFX_OMX_HEADER_LIBRARIES - common imported headers for all targets
#   MFX_OMX_LDFLAGS - common link flags for all targets

# =============================================================================
# Common definitions

MFX_OMX_CFLAGS := -DANDROID

# Android version preference:
ifneq ($(filter 11 11.% R ,$(PLATFORM_VERSION)),)
  MFX_ANDROID_VERSION:= MFX_R
endif
ifneq ($(filter 10 10.% Q ,$(PLATFORM_VERSION)),)
  MFX_ANDROID_VERSION:= MFX_Q
endif
ifneq ($(filter 9 9.% P ,$(PLATFORM_VERSION)),)
  MFX_ANDROID_VERSION:= MFX_P
endif
ifneq ($(filter 8.% O ,$(PLATFORM_VERSION)),)
  ifneq ($(filter 8.0.%,$(PLATFORM_VERSION)),)
    MFX_ANDROID_VERSION:= MFX_O
  else
    MFX_ANDROID_VERSION:= MFX_O_MR1
  endif
endif

ifeq ($(filter MFX,$(MFX_ANDROID_VERSION)),)
  $(error, Invalid MFX_ANDROID_VERSION '$(MFX_ANDROID_VERSION)')
endif

ifdef RESOURCES_LIMIT
  MFX_OMX_CFLAGS += \
    -DMFX_RESOURCES_LIMIT=$(RESOURCES_LIMIT)
endif

ifeq ($(MFX_ANDROID_PLATFORM),)
  MFX_ANDROID_PLATFORM:=$(TARGET_BOARD_PLATFORM)
endif

# We need to freeze Media SDK API to 1.26 on Android O
# because there is used old version of LibVA 2.0
ifneq ($(filter MFX_O MFX_O_MR1, $(MFX_ANDROID_VERSION)),)
  MFX_OMX_CFLAGS += -DMFX_VERSION=1026
endif

# Passing Android-dependency information to the code
MFX_OMX_CFLAGS += \
  -DMFX_ANDROID_VERSION=$(MFX_ANDROID_VERSION) \
  -DMFX_ANDROID_PLATFORM=$(MFX_ANDROID_PLATFORM)

ifeq ($(BOARD_USES_GRALLOC1),true)
  MFX_OMX_CFLAGS += -DMFX_OMX_USE_GRALLOC_1
  ifneq ($(filter MFX_P MFX_Q MFX_R,$(MFX_ANDROID_VERSION)),)
    # plugins should use PRIME buffer descriptor since Android P
    MFX_OMX_CFLAGS += -DMFX_OMX_USE_PRIME
  endif
endif

# Setting version information for the binaries
ifeq ($(MFX_VERSION),)
  MFX_VERSION = "6.0.010"
endif

MFX_OMX_CFLAGS += \
  -DMFX_FILE_VERSION=\"`echo $(MFX_VERSION) | cut -f 1 -d.``date +.%-y.%-m.%-d`\" \
  -DMFX_PRODUCT_VERSION=\"$(MFX_VERSION)\"

# Treat all warnings as error
MFX_OMX_CFLAGS += -Wall -Werror

#  Security
MFX_OMX_CFLAGS += \
  -fstack-protector \
  -fPIE -fPIC -pie \
  -O2 -D_FORTIFY_SOURCE=2 \
  -Wformat -Wformat-security \
  -fexceptions -frtti

# LibVA support.
MFX_OMX_CFLAGS_LIBVA := -DLIBVA_SUPPORT -DLIBVA_ANDROID_SUPPORT

ifneq ($(filter $(MFX_ANDROID_VERSION), MFX_O),)
  MFX_OMX_CFLAGS_LIBVA += -DANDROID_O
endif

ifneq ($(filter $(MFX_ANDROID_VERSION), MFX_Q MFX_R),)
  # HDR10 support on Android Q
  MFX_OMX_CFLAGS += -DHEVC10HDR_SUPPORT
  MFX_OMX_CFLAGS += -DENABLE_READ_SEI
endif

# Setting usual paths to include files
MFX_OMX_INCLUDES := $(LOCAL_PATH)/include

ifeq ($(BOARD_USES_GRALLOC1),true)
  MFX_OMX_INCLUDES += $(INTEL_MINIGBM)/cros_gralloc
endif

MFX_OMX_INCLUDES_LIBVA := $(TARGET_OUT_HEADERS)/libva

# Setting MediaSDK imported headers
MFX_OMX_HEADER_LIBRARIES := libmfx_headers

# Setting usual imported headers
ifneq ($(filter MFX_O_MR1 MFX_P MFX_Q MFX_R,$(MFX_ANDROID_VERSION)),)
  MFX_OMX_HEADER_LIBRARIES += \
    media_plugin_headers \
    libnativebase_headers \
    libui_headers \
    libhardware_headers
endif

ifneq ($(filter MFX_P MFX_Q MFX_R,$(MFX_ANDROID_VERSION)),)
  MFX_OMX_HEADER_LIBRARIES += \
    libbase_headers
endif

# Setting usual link flags
MFX_OMX_LDFLAGS := \
  -z noexecstack \
  -z relro -z now

# Setting vendor
LOCAL_MODULE_OWNER := intel

# Moving executables to proprietary location
LOCAL_PROPRIETARY_MODULE := true

// Copyright (c) 2011-2018 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef OMX_IntelVideoExt_h
#define OMX_IntelVideoExt_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Core.h>

typedef struct OMX_VIDEO_CONFIG_INTEL_BITRATETYPE {
     OMX_U32 nSize;
     OMX_VERSIONTYPE nVersion;
     OMX_U32 nPortIndex;
     OMX_U32 nMaxEncodeBitrate;    // Maximum bitrate
     OMX_U32 nTargetPercentage;    // Target bitrate as percentage of maximum bitrate; e.g. 95 is 95%
     OMX_U32 nWindowSize;          // Window size in milliseconds allowed for bitrate to reach target
     OMX_U32 nInitialQP;           // Initial QP for I frames
     OMX_U32 nMinQP;
     OMX_U32 nMaxQP;
     OMX_U32 nFrameRate;
     OMX_U32 nTemporalID;
} OMX_VIDEO_CONFIG_INTEL_BITRATETYPE;

typedef enum OMX_VIDEO_INTEL_CONTROL_RATE {
    OMX_Video_Intel_ControlRateVideoConferencingMode = OMX_Video_ControlRateVendorStartUnused + 1
} OMX_VIDEO_INTEL_CONTROL_RATE;

typedef struct OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_U32 nMaxNumberOfReferenceFrame;  // Maximum number of reference frames
     OMX_U32 nMaxWidth;                   // Maximum width of video
     OMX_U32 nMaxHeight;                  // Maximum height of video
} OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS;

typedef struct OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_U32 nISliceNumber;               // I frame slice number
     OMX_U32 nPSliceNumber;               // P frame slice number
} OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS;

typedef struct OMX_VIDEO_PARAM_INTEL_AVCVUI {
     OMX_U32 nSize;                       // Size of the structure
     OMX_VERSIONTYPE nVersion;            // OMX specification version
     OMX_U32 nPortIndex;                  // Port that this structure applies to
     OMX_BOOL  bVuiGeneration;            // Enable/disable VUI generation

} OMX_VIDEO_PARAM_INTEL_AVCVUI;

// Error reporting data structure
typedef struct OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnable;
} OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT;

#define MAX_ERR_NUM 10

typedef enum
{
    OMX_Decode_HeaderError   = 0,
    OMX_Decode_MBError       = 1,
    OMX_Decode_SliceMissing  = 2,
    OMX_Decode_RefMissing    = 3,
} OMX_VIDEO_DECODE_ERRORTYPE;

typedef struct OMX_VIDEO_ERROR_INFO {
    OMX_VIDEO_DECODE_ERRORTYPE type;
    OMX_U32 num_mbs;
    union {
        struct {OMX_U32 start_mb; OMX_U32 end_mb;} mb_pos;
    } error_data;
} OMX_VIDEO_ERROR_INFO;

typedef struct OMX_VIDEO_ERROR_BUFFER {
    OMX_U32 errorNumber;   // Error number should be no more than MAX_ERR_NUM
    OMX_S64 timeStamp;      // presentation time stamp
    OMX_VIDEO_ERROR_INFO errorArray[MAX_ERR_NUM];
} OMX_VIDEO_ERROR_BUFFER __attribute__((aligned(8)));

typedef struct OMX_VIDEO_OUTPUT_ERROR_BUFFERS {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nErrorBufIndex;
    OMX_VIDEO_ERROR_BUFFER errorBuffers;
} OMX_VIDEO_OUTPUT_ERROR_BUFFERS;

// Request OMX to allocate a black frame to video mute feature
typedef struct OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_PTR nFramePointer;
} OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER;

typedef struct OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIndex;
    OMX_U8* pNativeHandle;
} OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE;

// The structure is used to set HRD parameters on the encoder during
// initialization/reset, with the following parameter structure
typedef struct OMX_VIDEO_PARAM_HRD_PARAM
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nHRDBufferSize;      // nHRDBufferSize is used to configure buffer size for HRD model (in kilo-bytes).
    OMX_U32 nHRDInitialFullness; // nHRDInitialFullness is used to configure initial fullness for HRD model (in kilo-bytes).
    OMX_BOOL bWriteHRDSyntax;    // bWriteHRDSyntax is used to enable HRD syntax writing to bitstream
} OMX_VIDEO_PARAM_HRD_PARAM;

// The structure is used to set maximum picture size parameter on
// the encoder during initialization/reset, with the following parameter structure
typedef struct OMX_VIDEO_PARAM_MAX_PICTURE_SIZE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nMaxPictureSize; // nMaxPictureSize is used to configure maximum frame size (in bytes).
} OMX_VIDEO_PARAM_MAX_PICTURE_SIZE;

// The structure is used to configure target usage (quility vs speed)
typedef struct OMX_VIDEO_PARAM_TARGET_USAGE
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nTargetUsage; // nTargetUsage is used to configure target usage (quility vs speed) parameter. Values range is [1...7].
                          //1 means the best quality. 7 means the best speed
} OMX_VIDEO_PARAM_TARGET_USAGE;

// The structure is used to add user date to the next encoding
// AVC frame SEI Nalu during encoding with the following parameter structure
typedef struct OMX_VIDEO_CONFIG_USERDATA
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nUserDataSize; // nUserDataSize is used to inform the size of the user data (in bytes)
    OMX_U8 *pUserData;     // pUserData is a pointer to the user data needed to be attached to the next encoding frame.
} OMX_VIDEO_CONFIG_USERDATA;

// The structure is used to set encoder cropping values
typedef struct OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nCropX;
    OMX_U32 nCropY;
    OMX_U32 nCropW;
    OMX_U32 nCropH;
} OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM;

typedef struct OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM
{
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U16 nAspectRatioW;
    OMX_U16 nAspectRatioH;
    OMX_U16 nVideoFormat;
    OMX_U16 nColourDescriptionPresent;
    OMX_U16 nColourPrimaries;
    OMX_U16 nFixedFrameRate;
    OMX_U16 nPicStructPresent;
    OMX_U16 nLowDelayHRD;
    OMX_BOOL bVideoFullRange;
    OMX_U32 nReserved[4];
}
OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM;

// The structure is used to configure encoding dirty rect
typedef struct OMX_VIDEO_CONFIG_DIRTY_RECT {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nNumRectangles;
    OMX_CONFIG_RECTTYPE* pRectangles;
} OMX_VIDEO_CONFIG_DIRTY_RECT;

// The structure is used to control dummy frames inserting
typedef struct OMX_CONFIG_DUMMYFRAME {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bInsertDummyFrame;
} OMX_CONFIG_DUMMYFRAME;

// The structure is used to configure number of slice number
typedef struct OMX_CONFIG_NUMSLICE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nSliceNumber;
} OMX_CONFIG_NUMSLICE;

// The structure is used to enable internal skip frame
typedef struct OMX_CONFIG_ENABLE_INTERNAL_SKIP {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bEnableInternalSkip;
} OMX_CONFIG_ENABLE_INTERNAL_SKIP;

// The structure is used to turn off bitrate limit
typedef struct OMX_CONFIG_BITRATELIMITOFF {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bBitrateLimitOff;
} OMX_CONFIG_BITRATELIMITOFF;

// The structure is used to configure number of reference frame
typedef struct OMX_CONFIG_NUMREFFRAME {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nRefFrame;
} OMX_CONFIG_NUMREFFRAME;

// The structure is used to configure goppicsize
typedef struct OMX_CONFIG_GOPPICSIZE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nGopPicSize;
} OMX_CONFIG_GOPPICSIZE;

// The structure is used to configure idrinterval
typedef struct OMX_CONFIG_IDRINTERVAL {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nIdrInterval;
} OMX_CONFIG_IDRINTERVAL;

// The structure is used to configure goprefdist
typedef struct OMX_CONFIG_GOPREFDIST {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nGopRefDist;
} OMX_CONFIG_GOPREFDIST;

// The structure is used to configure low power
typedef struct OMX_CONFIG_LOWPOWER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bLowPower;
} OMX_CONFIG_LOWPOWER;

// The structure is used to configure DecodedOrder
typedef struct OMX_CONFIG_DECODEDORDER {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDecodedOrder;
} OMX_CONFIG_DECODEDORDER;

// The structure is used to disable deblocking
typedef struct OMX_CONFIG_DISABLEDEBLOCKINGIDC {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDisableDeblockingIdc;
} OMX_CONFIG_DISABLEDEBLOCKINGIDC;

// Set temporal layer count
typedef struct OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_U32 nTemproalLayerCount;
} OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT;

// SFC
typedef struct OMX_VIDEO_PARAM_SFC {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;                // Allow SFC apply on both input and output of OMX component.  Output is for decoding, and input is for video encoding
    OMX_BOOL bEnableSFC;               // True: request to enable SFC (scalar/format conversion): False: just return
    OMX_COLOR_FORMATTYPE outputFormat; // It is for future extension,  for Nuplayer usage, ignore this
    OMX_S32 nRotation;                 // It is for future extension. If ISV can get the rotation information, it is better
    OMX_U32 nOutputWidth;              // nOutputWidth is from ISV for downscaling
    OMX_U32 nOutputHeight;             // nOutputHeight is from ISV for downscaling
} OMX_VIDEO_PARAM_SFC;

#define OMX_BUFFERFLAG_TFF 0x00010000
#define OMX_BUFFERFLAG_BFF 0x00020000

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_VideoExt_h */

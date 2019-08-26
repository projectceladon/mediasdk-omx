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

#ifndef OMX_IntelIndexExt_h
#define OMX_IntelIndexExt_h

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <OMX_Index.h>

typedef enum OMX_INTELINDEXEXTTYPE {
    OMX_IndexIntelStartUsed = OMX_IndexVendorStartUnused + 1,
    OMX_IndexConfigIntelBitrate,                    /**< reference: OMX_VIDEO_CONFIG_INTEL_BITRATETYPE */
    OMX_IndexParamIntelAVCDecodeSettings,           /**< reference: OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS */
    OMX_IndexConfigIntelSliceNumbers,               /**< reference: OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS */
    OMX_IndexParamIntelAVCVUI,                      /**< reference: OMX_VIDEO_PARAM_INTEL_AVCVUI */

    /* Error report */
    OMX_IndexExtEnableErrorReport,                  /**<reference: OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT */
    OMX_IndexExtOutputErrorBuffers,                 /**<reference: OMX_VIDEO_OUTPUT_ERROR_BUFFERS */
    OMX_IndexExtRequestBlackFramePointer,           /**<reference: OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER*/
    OMX_IndexExtDecoderBufferHandle,                /**<reference: OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE*/

    /* LTR and Temporal Layer for AVC */
    OMX_IndexExtTemporalLayerCount,                 /**<reference: OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT */

    OMX_IntelIndexExtMax = 0x7FFFFFFF
} OMX_INTELINDEXEXTTYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMX_IndexExt_h */

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

#ifndef __MFX_OMX_GEN_H__
#define __MFX_OMX_GEN_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_generic.h"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_PRODUCT_NAME \
    "Intel(r) Media SDK OpenMAX Hardware Integration Layer Components"

/*------------------------------------------------------------------------------*/

static const OMX_VIDEO_PARAM_PORTFORMATTYPE g_srf_input_formats[] =
{
    MFX_OMX_DECLARE_SRF_INPUT_FORMAT(OMX_COLOR_FormatYUV420SemiPlanar),
    MFX_OMX_DECLARE_SRF_INPUT_FORMAT(OMX_COLOR_FormatAndroidOpaque)
};

/*------------------------------------------------------------------------------*/

static const OMX_VIDEO_PARAM_PORTFORMATTYPE g_srf_output_formats[] =
{
    MFX_OMX_DECLARE_SRF_OUTPUT_FORMAT(OMX_COLOR_FormatYUV420SemiPlanar),
    MFX_OMX_DECLARE_SRF_OUTPUT_FORMAT((OMX_COLOR_FORMATTYPE)OMX_INTEL_COLOR_Format_NV12),
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_h264_profile_levels[] =
{
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileExtended, OMX_VIDEO_AVCLevel51 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 }
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_h265_profile_levels[] =
{
    { MFX_OMX_VIDEO_HEVCProfileMain, MFX_OMX_VIDEO_HEVCMainTierLevel51 },
    { MFX_OMX_VIDEO_HEVCProfileMain10, MFX_OMX_VIDEO_HEVCMainTierLevel51 }
#ifdef HDR_SEI_PAYLOAD
    ,{ MFX_OMX_VIDEO_HEVCProfileMain10HDR10, MFX_OMX_VIDEO_HEVCMainTierLevel51 }
#endif
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_mp2_profile_levels[] =
{
    { OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelLL }
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_vc1_profile_levels[] =
{ /* level is not applicable here */
    { OMX_VIDEO_WMVFormatUnused, 0 }
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_vp8_profile_levels[] =
{
    { OMX_VIDEO_VP8ProfileMain, OMX_VIDEO_VP8Level_Version0 }
};

/*------------------------------------------------------------------------------*/

static const MfxOmxProfileLevelTable g_vp9_profile_levels[] =
{
    { OMX_VIDEO_VP9Profile0, OMX_VIDEO_VP9Level52 }
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_GEN_H__

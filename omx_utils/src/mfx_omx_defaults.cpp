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

#include "mfx_omx_defaults.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_defaults"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

void mfx_omx_set_defaults_mfxFrameInfo(mfxFrameInfo* info)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (!info) return;
    memset(info, 0, sizeof(mfxFrameInfo));
}

/*------------------------------------------------------------------------------*/

void mfx_omx_set_defaults_mfxVideoParam_dec(mfxVideoParam* params)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxU32 CodecId = 0;

    if (!params) return;
    CodecId = params->mfx.CodecId;

    memset(params, 0, sizeof(mfxVideoParam));
    params->AsyncDepth = 0;
    params->mfx.CodecId = CodecId;
    params->mfx.NumThread = (mfxU16)mfx_omx_get_cpu_num();
    MFX_OMX_AT__mfxVideoParam_dec((*params))
}

/*------------------------------------------------------------------------------*/

void mfx_omx_set_defaults_mfxVideoParam_vpp(mfxVideoParam* params)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (!params) return;
    memset(params, 0, sizeof(mfxVideoParam));
    /** @todo For vpp it is needed to set extended parameters also. */
}

/*------------------------------------------------------------------------------*/

void mfx_omx_set_defaults_mfxVideoParam_enc(mfxVideoParam* params)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxU32 CodecId = 0;

    if (!params) return;
    CodecId = params->mfx.CodecId;

    memset(params, 0, sizeof(mfxVideoParam));
    params->mfx.CodecId = CodecId;
    params->mfx.NumThread = (mfxU16)mfx_omx_get_cpu_num();
    switch (params->mfx.CodecId)
    {
    case MFX_CODEC_AVC:
        // Setting mimimum number of parameters:
        //  - TargetUsage: best speed: to mimimize number of used features
        //  - RateControlMethod: constant bitrate
        //  - PicStruct: progressive
        //  - TargetKbps: 2222: some
        //  - GopRefDist: 1: to exclude B-frames which can be unsupported on some devices
        //  - GopPicSize: 15: some
        //  - NumSlice: 1
        params->mfx.CodecProfile = MFX_PROFILE_AVC_CONSTRAINED_BASELINE;
        params->mfx.CodecLevel = MFX_LEVEL_AVC_51;
        params->mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
        params->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        params->mfx.TargetKbps = 2222;
        params->mfx.RateControlMethod   = MFX_RATECONTROL_CBR;
        params->mfx.GopRefDist = 1;
        params->mfx.GopPicSize = 15;
        params->mfx.NumSlice = 1;
        break;
    case MFX_CODEC_HEVC:
        params->mfx.CodecProfile = MFX_PROFILE_HEVC_MAIN;
        params->mfx.CodecLevel = MFX_LEVEL_HEVC_6;
        params->mfx.TargetUsage = MFX_TARGETUSAGE_BEST_SPEED;
        params->mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
        params->mfx.TargetKbps = 3000;
        params->mfx.RateControlMethod = MFX_RATECONTROL_CBR;
        params->mfx.GopPicSize = 16;
        params->mfx.GopRefDist = 1;
        params->mfx.NumSlice = 1;
        params->mfx.NumRefFrame = 1;
        break;
    default:
        break;
    };
    MFX_OMX_AT__mfxVideoParam_enc((*params))
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

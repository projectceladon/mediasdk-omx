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

#ifndef __MFX_OMX_STRUCTURES_H__
#define __MFX_OMX_STRUCTURES_H__

#include "mfx_omx_types.h"
#include "mfx_omx_vm.h"

/*------------------------------------------------------------------------------*/

// omx -> mfx (mfxEncoderCtrl)
template<typename T>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxEncodeCtrlWrapper& mfxctrl,
    const T& omxparams);

// omx -> mfx (mfxVideoParam)
template<typename T>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const T& omxparams);

// mfx -> omx
template<typename T>
OMX_ERRORTYPE mfx2omx_config(
    T& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams);

// omx -> mfx (SetConfig)
template<typename T>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const MfxOmxInputConfig& curconfig,
    const T* omxparams);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern OMX_VIDEO_AVCPROFILETYPE mfx2omx_avc_profile(mfxU16 mfx_profile);
extern mfxU16 omx2mfx_avc_profile(OMX_U32 omx_profile);

extern OMX_VIDEO_AVCLEVELTYPE mfx2omx_avc_level(mfxU16 mfx_profile);
extern mfxU16 omx2mfx_avc_level(OMX_U32 omx_profile);

extern OMX_VIDEO_CONTROLRATETYPE mfx2omx_brc(mfxU16 mfx_brc);
extern mfxU16 omx2mfx_brc(OMX_VIDEO_CONTROLRATETYPE omx_brc);

extern OMX_STRING mfx_omx_coding2mime_type(OMX_VIDEO_CODINGTYPE type);
extern OMX_STRING mfx_omx_color2mime_type(OMX_COLOR_FORMATTYPE type);

extern MfxOmxEncodeCtrlWrapper* CreateEncodeCtrlWrapper();
extern  MfxOmxVideoParamsWrapper* CreateVideoParamsWrapper(
    const MfxOmxVideoParamsWrapper& ref,
    bool bNeedResetExtBuffer = false
);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // #ifndef __MFX_OMX_STRUCTURES_H__

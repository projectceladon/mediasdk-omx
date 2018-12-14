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

#ifndef __MFX_OMX_DEFAULTS_H__
#define __MFX_OMX_DEFAULTS_H__

#include "mfx_omx_utils.h"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

/**
 * Fills mfxFrameInfo structure with default values.
 * @param[out] info structure to initialize
 */
extern void mfx_omx_set_defaults_mfxFrameInfo(mfxFrameInfo* info);
/**
 * Fills mfxVideoParam with default values assuming that it will be used in
 * decoder initialization. Since parameters can be different for different
 * decoders caller should fill params->mfx.CodecId field prior calling this
 * function,
 * @param[in,out] params parameters to initialize
 */
extern void mfx_omx_set_defaults_mfxVideoParam_dec(mfxVideoParam* params);
/**
 * Fills mfxVideoParam with default values assuming that it will be used in
 * vpp initialization.
 * @param[in,out] params parameters to initialize
 */
extern void mfx_omx_set_defaults_mfxVideoParam_vpp(mfxVideoParam* params);
/**
 * Fills mfxVideoParam with default values assuming that it will be used in
 * encoder initialization. Since parameters can be different for different
 * encoders caller should fill params->mfx.CodecId field prior calling this
 * function,
 * @param[in,out] params parameters to initialize
 */
extern void mfx_omx_set_defaults_mfxVideoParam_enc(mfxVideoParam* params);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_DEFAULTS_H__

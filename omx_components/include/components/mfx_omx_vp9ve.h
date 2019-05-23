// Copyright (c) 2019 Intel Corporation
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

#ifndef __MFX_OMX_VP9VE_H__
#define __MFX_OMX_VP9VE_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_generic.h"
#include "mfx_omx_gen.h"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_VP9VE_NAME "OMX.Intel.hw_ve.vp9"

/*------------------------------------------------------------------------------*/

#define g_vp9ve_input_formats g_srf_input_formats

static const OMX_VIDEO_PARAM_PORTFORMATTYPE g_vp9ve_output_formats[] =
{
    MFX_OMX_DECLARE_BST_OUTPUT_FORMAT(OMX_VIDEO_CodingVP9)
};

#define g_vp9ve_profile_levels g_vp9_profile_levels

#define mfx_omx_create_vp9ve_ports mfx_omx_create_port

static const char* g_vp9ve_roles[] =
{
    "video_encoder.vp9"
};

/*------------------------------------------------------------------------------*/

MFX_OMX_DECLARE_ENC_PORTS(vp9ve);

#define MFX_OMX_DECLARE_VP9VE \
    MFX_OMX_DECLARE_COMPONENT(MFX_OMX_VP9VE_NAME, vp9ve)

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_VP9VE_H__

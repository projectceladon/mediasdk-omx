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

/********************************************************************************

Defined functions:
  - MFX_OMX_ComponentInit - so entry point; component create function

*********************************************************************************/

// this will force exporting of OMX functions
#define __OMX_EXPORTS
// this will define debug file to dump log (if debug printing is enabled)
#define MFX_OMX_FILE_INIT

/*------------------------------------------------------------------------------*/

#include "mfx_omx_utils.h"
#include "mfx_omx_generic.h"
#include "mfx_omx_component.h"

/*------------------------------------------------------------------------------*/

#include "components/mfx_omx_gen.h"
#include "components/mfx_omx_h264vd.h"
#include "components/mfx_omx_h265vd.h"
#include "components/mfx_omx_h264ve.h"
#include "components/mfx_omx_mp2vd.h"
#include "components/mfx_omx_vc1vd.h"
#include "components/mfx_omx_vp8vd.h"
#include "components/mfx_omx_vp9vd.h"
#include "components/mfx_omx_h265ve.h"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

MFX_OMX_DECLARE_COMPONENT_VERSION();

/*------------------------------------------------------------------------------*/

static MfxOmxComponentRegData g_mfx_omx_components[] =
{
    MFX_OMX_DECLARE_H264VD
    ,MFX_OMX_DECLARE_H265VD
    ,MFX_OMX_DECLARE_H264VE
    ,MFX_OMX_DECLARE_MP2VD
    ,MFX_OMX_DECLARE_VC1VD
    ,MFX_OMX_DECLARE_VP8VD
    ,MFX_OMX_DECLARE_VP9VD
    ,MFX_OMX_DECLARE_H265VE
};

/*------------------------------------------------------------------------------*/

MFX_OMX_DECLARE_COMPONENT_ENTRY_POINT()

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */




// Copyright (c) 2014-2018 Intel Corporation
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

#ifndef __MFX_OMX_VPP_WRAPP_H__
#define __MFX_OMX_VPP_WRAPP_H__

#include <stdio.h>
#include <utility>
#include <vector>

#include "mfx_omx_defs.h"
#include "mfx_omx_types.h"

/*------------------------------------------------------------------------------*/

#define VPP_MAX_SRF_NUM 10

/*------------------------------------------------------------------------------*/

enum MfxOmxConversion
{
    CONVERT_NONE,
    NV12_TO_ARGB,
    ARGB_TO_NV12
};

/*------------------------------------------------------------------------------*/

struct MfxOmxVppWrappParam
{
    MFXVideoSession   *session;
    mfxFrameInfo      *frame_info;
    mfxFrameAllocator *allocator;

    MfxOmxConversion   conversion;
};

/*------------------------------------------------------------------------------*/

class MfxOmxVppWrapp
{
public:
    MfxOmxVppWrapp(void);
    ~MfxOmxVppWrapp(void);

    mfxStatus Init(MfxOmxVppWrappParam *param);
    mfxStatus Close(void);
    mfxStatus ProcessFrameVpp(mfxFrameSurface1 *in_srf, mfxFrameSurface1 **out_srf);
    void SetEncodeCtrl(MfxOmxEncodeCtrlWrapper* pEncodeCtrlWrap, mfxFrameSurface1 *pFrameSurface);

protected:
    mfxStatus FillVppParams(mfxFrameInfo *frame_info, MfxOmxConversion conversion);
    mfxStatus AllocateOneSurface(void);

    mfxStatus DumpSurface(mfxFrameSurface1 *surface, FILE *file);

    MFXVideoVPP *m_pVPP;
    MFXVideoSession *m_session;
    mfxVideoParam m_vppParam;
    mfxFrameAllocator m_allocator;

    mfxFrameAllocResponse m_responses[VPP_MAX_SRF_NUM];
    mfxFrameSurface1 m_vppSrf[VPP_MAX_SRF_NUM];
    mfxU32 m_numVppSurfaces;

    typedef std::pair<MfxOmxEncodeCtrlWrapper*, mfxFrameSurface1*> EncodeCtrlPair;
    std::vector<EncodeCtrlPair> m_EncodeCtrls;

    // debug file
    FILE* m_dbg_vppout;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVppWrapp)
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_VPP_WRAPP_H__

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

#include "mfx_omx_vpp_wrapp.h"
#include "mfx_omx_defs.h"
#include "mfx_omx_utils.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_vpp_wrapp"

#define MFX_OMX_VPPOUT_FILE "/data/mfx/mfx_omx_vppout"

/*------------------------------------------------------------------------------*/

MfxOmxVppWrapp::MfxOmxVppWrapp(void):
    m_pVPP(NULL),
    m_session(NULL),
    m_numVppSurfaces(0),
    m_dbg_vppout(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_ZERO_MEMORY(m_vppParam);
    MFX_OMX_ZERO_MEMORY(m_allocator);
    MFX_OMX_ZERO_MEMORY(m_responses);
}

/*------------------------------------------------------------------------------*/

MfxOmxVppWrapp::~MfxOmxVppWrapp(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    Close();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::Init(MfxOmxVppWrappParam *param)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MFX_ERR_NONE;

    if (!param || !param->session || !param->allocator || !param->frame_info)
        sts = MFX_ERR_NULL_PTR;

    if (MFX_ERR_NONE == sts)
    {
        MFX_OMX_COPY(m_allocator, *param->allocator);
        m_session = param->session;

        MFX_OMX_NEW(m_pVPP, MFXVideoVPP(*m_session));
        if(!m_pVPP) sts = MFX_ERR_UNKNOWN;

        if (MFX_ERR_NONE == sts) sts = FillVppParams(param->frame_info, param->conversion);
        MFX_OMX_AT__mfxFrameInfo(m_vppParam.vpp.In);
        MFX_OMX_AT__mfxFrameInfo(m_vppParam.vpp.Out);

        if (MFX_ERR_NONE == sts) sts = m_pVPP->Init(&m_vppParam);
    }

    if (MFX_ERR_NONE == sts) sts = AllocateOneSurface();

#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
        if (MFX_ERR_NONE == sts)
        {
            m_dbg_vppout = fopen(MFX_OMX_VPPOUT_FILE, "w");
        }
#endif

    if (MFX_ERR_NONE != sts) Close();

    MFX_OMX_AUTO_TRACE_I32(sts);
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::Close(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MFX_ERR_NONE;

    std::vector<EncodeCtrlPair>::iterator iter = m_EncodeCtrls.begin();
    while (iter != m_EncodeCtrls.end())
    {
        EncodeCtrlPair pair = *iter;
        if (0 == pair.second->Data.Locked)
        {
            MfxOmxEncodeCtrlWrapper* wrap = pair.first;
            MFX_OMX_DELETE(wrap);
            iter = m_EncodeCtrls.erase(iter);
        }
        else ++iter;
    }

    if (m_pVPP)
    {
        sts = m_pVPP->Close();
        MFX_OMX_DELETE(m_pVPP);
        m_pVPP = NULL;
    }

    MFX_OMX_AUTO_TRACE_I32(m_numVppSurfaces);
    if (m_numVppSurfaces)
    {
        for (mfxU32 i = 0; i < m_numVppSurfaces; i++)
        {
            m_allocator.Free(m_allocator.pthis, &m_responses[i]);
        }
    }

    MFX_OMX_ZERO_MEMORY(m_responses);
    MFX_OMX_ZERO_MEMORY(m_vppSrf);
    m_numVppSurfaces = 0;
    m_session = NULL;

    if (m_dbg_vppout) fclose(m_dbg_vppout);

    MFX_OMX_AUTO_TRACE_I32(sts);
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::FillVppParams(mfxFrameInfo *frame_info, MfxOmxConversion conversion)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MFX_ERR_NONE;

    if (!frame_info) sts = MFX_ERR_NULL_PTR;

    if (MFX_ERR_NONE == sts)
    {
        MFX_OMX_ZERO_MEMORY(m_vppParam);
        m_vppParam.AsyncDepth = 1;
        m_vppParam.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;
        MFX_OMX_COPY(m_vppParam.vpp.In, *frame_info);
        MFX_OMX_COPY(m_vppParam.vpp.Out, *frame_info);

        switch (conversion)
        {
            case ARGB_TO_NV12:
                if (MFX_FOURCC_RGB4 != m_vppParam.vpp.In.FourCC)
                    sts = MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

                m_vppParam.vpp.Out.FourCC = MFX_FOURCC_NV12;
                break;

            case NV12_TO_ARGB:
                if (MFX_FOURCC_NV12 != m_vppParam.vpp.In.FourCC)
                    sts = MFX_ERR_INCOMPATIBLE_VIDEO_PARAM;

                m_vppParam.vpp.Out.FourCC = MFX_FOURCC_RGB4;
                break;

            case CONVERT_NONE:
                break;
        }

        if (MFX_ERR_NONE != sts)
            MFX_OMX_ZERO_MEMORY(m_vppParam);
    }

    MFX_OMX_AUTO_TRACE_I32(sts);
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::AllocateOneSurface(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MFX_ERR_NONE;

    if (m_numVppSurfaces >= VPP_MAX_SRF_NUM) sts = MFX_ERR_UNKNOWN;

    if (MFX_ERR_NONE == sts)
    {
        mfxFrameAllocRequest request;
        MFX_OMX_ZERO_MEMORY(request);
        MFX_OMX_COPY(request.Info, m_vppParam.vpp.Out);
        request.NumFrameMin = 1;
        request.NumFrameSuggested = 1;
        request.Type = MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET | MFX_MEMTYPE_FROM_VPPOUT;

        sts = m_allocator.Alloc(m_allocator.pthis, &request, &m_responses[m_numVppSurfaces]);
    }

    if (MFX_ERR_NONE == sts)
    {
        MFX_OMX_ZERO_MEMORY(m_vppSrf[m_numVppSurfaces]);
        MFX_OMX_COPY(m_vppSrf[m_numVppSurfaces].Info, m_vppParam.vpp.Out);
        m_vppSrf[m_numVppSurfaces].Data.MemId = m_responses[m_numVppSurfaces].mids[0];
        m_numVppSurfaces++;
    }

    MFX_OMX_AUTO_TRACE_I32(sts);
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::ProcessFrameVpp(mfxFrameSurface1 *in_srf, mfxFrameSurface1 **out_srf)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MFX_ERR_NONE;
    mfxFrameSurface1* outSurface = NULL;
    mfxSyncPoint syncp;

    std::vector<EncodeCtrlPair>::iterator iter = m_EncodeCtrls.begin();
    while (iter != m_EncodeCtrls.end())
    {
        EncodeCtrlPair pair = *iter;
        if (0 == pair.second->Data.Locked)
        {
            MfxOmxEncodeCtrlWrapper* wrap = pair.first;
            MFX_OMX_DELETE(wrap);
            iter = m_EncodeCtrls.erase(iter);
        }
        else ++iter;
    }

    if (!in_srf || !out_srf) return MFX_ERR_UNKNOWN;


    for (mfxU32 i = 0; i < m_numVppSurfaces; i++)
    {
        if (false == m_vppSrf[i].Data.Locked)
        {
            outSurface = &m_vppSrf[i];
            break;
        }
    }

    if (m_numVppSurfaces < VPP_MAX_SRF_NUM && NULL == outSurface)
    {
        sts = AllocateOneSurface();
        if (MFX_ERR_NONE == sts) outSurface = &m_vppSrf[m_numVppSurfaces-1]; // just created outSurface
    }

    if (outSurface)
    {
        sts = m_pVPP->RunFrameVPPAsync(in_srf, outSurface, NULL, &syncp);
        if (MFX_ERR_NONE == sts) sts = m_session->SyncOperation(syncp, MFX_OMX_INFINITE);
    }
    else sts = MFX_ERR_MORE_SURFACE;

    if (MFX_ERR_NONE == sts)
    {
        *out_srf = outSurface;

#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
        if (m_dbg_vppout) DumpSurface(outSurface, m_dbg_vppout);
#endif
    }

    MFX_OMX_AUTO_TRACE_I32(sts);
    return sts;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVppWrapp::SetEncodeCtrl(MfxOmxEncodeCtrlWrapper* pEncodeCtrlWrap, mfxFrameSurface1 *pFrameSurface)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (pEncodeCtrlWrap && pFrameSurface)
    {
        m_EncodeCtrls.push_back(std::make_pair(pEncodeCtrlWrap, pFrameSurface));
    }
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVppWrapp::DumpSurface(mfxFrameSurface1 *surface, FILE *file)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (surface && file)
    {
        mfx_res = m_allocator.Lock(m_allocator.pthis, surface->Data.MemId, &(surface->Data));
        if (MFX_ERR_NONE == mfx_res)
        {
            switch(surface->Info.FourCC)
            {
                case MFX_FOURCC_NV12:
                    mfx_omx_dump_YUV_from_NV12_data(file, &(surface->Data), &(surface->Info), surface->Data.Pitch);
                    break;
                case MFX_FOURCC_RGB4:
                    mfx_omx_dump_RGB_from_RGB4_data(file, &(surface->Data), &(surface->Info), surface->Data.Pitch);
                    break;
                default:
                    break;
            }
            mfx_res = m_allocator.Unlock(m_allocator.pthis, surface->Data.MemId, &(surface->Data));
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

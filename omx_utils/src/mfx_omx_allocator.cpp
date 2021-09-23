// Copyright (c) 2013-2018 Intel Corporation
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

#include "mfx_omx_allocator.h"

/*------------------------------------------------------------------------------*/

static const mfxU32 MEMTYPE_FROM_MASK =
    MFX_MEMTYPE_FROM_ENCODE |
    MFX_MEMTYPE_FROM_DECODE |
    MFX_MEMTYPE_FROM_VPPIN |
    MFX_MEMTYPE_FROM_VPPOUT;

/*------------------------------------------------------------------------------*/

inline MfxOmxFrameAllocator* _wrapper_get_allocator(mfxHDL pthis)
{
    return (MfxOmxFrameAllocator*)pthis;
}

static mfxStatus _wrapper_alloc(
  mfxHDL pthis, mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    MfxOmxFrameAllocator* a = _wrapper_get_allocator(pthis);
    return (a)? a->AllocFrames(request, response): MFX_ERR_INVALID_HANDLE;
}

static mfxStatus _wrapper_lock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    MfxOmxFrameAllocator* a = _wrapper_get_allocator(pthis);
    return (a)? a->LockFrame(mid, ptr): MFX_ERR_INVALID_HANDLE;
}

static mfxStatus _wrapper_unlock(mfxHDL pthis, mfxMemId mid, mfxFrameData *ptr)
{
    MfxOmxFrameAllocator* a = _wrapper_get_allocator(pthis);
    return (a)? a->UnlockFrame(mid, ptr): MFX_ERR_INVALID_HANDLE;
}

static mfxStatus _wrapper_free(mfxHDL pthis, mfxFrameAllocResponse *response)
{
    MfxOmxFrameAllocator* a = _wrapper_get_allocator(pthis);
    return (a)? a->FreeFrames(response): MFX_ERR_INVALID_HANDLE;
}

static mfxStatus _wrapper_get_hdl(mfxHDL pthis, mfxMemId mid, mfxHDL *handle)
{
    MfxOmxFrameAllocator* a = _wrapper_get_allocator(pthis);
    return (a)? a->GetFrameHDL(mid, handle): MFX_ERR_INVALID_HANDLE;
}

/*------------------------------------------------------------------------------*/

MfxOmxFrameAllocator::MfxOmxFrameAllocator()
{
    mfxFrameAllocator& allocator = *((mfxFrameAllocator*)this);

    MFX_OMX_ZERO_MEMORY(allocator);
    allocator.pthis = this;
    allocator.Alloc = _wrapper_alloc;
    allocator.Free = _wrapper_free;
    allocator.Lock = _wrapper_lock;
    allocator.Unlock = _wrapper_unlock;
    allocator.GetHDL = _wrapper_get_hdl;

    MFX_OMX_ZERO_MEMORY(m_DecoderResponse);
#ifdef OMX_ENABLE_DECVPP
    MFX_OMX_ZERO_MEMORY(m_VPPResponse);
#endif
}

MfxOmxFrameAllocator::~MfxOmxFrameAllocator()
{
}

mfxStatus MfxOmxFrameAllocator::CheckRequestType(mfxFrameAllocRequest *pRequest)
{
    if (!pRequest)  return MFX_ERR_NULL_PTR;

    // check that Media SDK component is specified in request
    if ((pRequest->Type & MEMTYPE_FROM_MASK) != 0)
        return MFX_ERR_NONE;
    else
        return MFX_ERR_UNSUPPORTED;
}

mfxStatus MfxOmxFrameAllocator::AllocFrames(mfxFrameAllocRequest *pRequest, mfxFrameAllocResponse *pResponse)
{
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!pRequest || !pResponse) return MFX_ERR_NULL_PTR;
    if ((MFX_ERR_NONE == mfx_res) && (0 == pRequest->NumFrameSuggested)) return MFX_ERR_MEMORY_ALLOC;

    if (MFX_ERR_NONE == mfx_res) mfx_res = CheckRequestType(pRequest);
    if (MFX_ERR_NONE == mfx_res)
    {
#ifdef OMX_ENABLE_DECVPP
        bool bDecoderResponse = (pRequest->Type & MFX_MEMTYPE_FROM_DECODE);
        bool bVPPResponse = (pRequest->Type & MFX_MEMTYPE_FROM_VPPOUT) &&
                            (pRequest->Type & MFX_MEMTYPE_EXTERNAL_FRAME);
        ALOGI("%s, bDecoderResponse = %d, NumFrameSuggested = %d, NumFrameActual= %d, bVPPResponse=%d, NumFrameActual= %d", __func__, 
			bDecoderResponse, pRequest->NumFrameSuggested, m_DecoderResponse.response.NumFrameActual,
			bVPPResponse, m_VPPResponse.response.NumFrameActual);

        if (bDecoderResponse)
        {
            if (0 != m_DecoderResponse.response.NumFrameActual)
            {
                if (pRequest->NumFrameSuggested > m_DecoderResponse.response.NumFrameActual)
                {
                    mfx_res = MFX_ERR_MEMORY_ALLOC;
                }
                else
                {
                    *pResponse = m_DecoderResponse.response;
                }
            }
            else
            {
                mfx_res = AllocImpl(pRequest, pResponse);
                if (MFX_ERR_NONE == mfx_res)
                {
                    m_DecoderResponse.response = *pResponse;
                }
            }

            if (MFX_ERR_NONE == mfx_res) ++(m_DecoderResponse.refcount);
        }
        else
        {
            if (bVPPResponse && (0 != m_VPPResponse.response.NumFrameActual))
            {
                if (pRequest->NumFrameSuggested > m_VPPResponse.response.NumFrameActual)
                {
                    mfx_res = MFX_ERR_MEMORY_ALLOC;
                }
                else
                {
                    *pResponse = m_VPPResponse.response;
                }
            }
            else
            {
                mfx_res = AllocImpl(pRequest, pResponse);
                if (bVPPResponse && MFX_ERR_NONE == mfx_res)
                {
                    m_VPPResponse.response = *pResponse;
                }
            }

            if ((MFX_ERR_NONE == mfx_res) && bVPPResponse) ++(m_VPPResponse.refcount);
        }
#else
        bool bDecoderResponse = (pRequest->Type & MFX_MEMTYPE_EXTERNAL_FRAME) &&
                                (pRequest->Type & MFX_MEMTYPE_FROM_DECODE);

        if (bDecoderResponse && (0 != m_DecoderResponse.response.NumFrameActual))
        {
            if (pRequest->NumFrameSuggested > m_DecoderResponse.response.NumFrameActual)
            {
                mfx_res = MFX_ERR_MEMORY_ALLOC;
            }
            else
            {
                *pResponse = m_DecoderResponse.response;
            }
        }
        else
        {
            mfx_res = AllocImpl(pRequest, pResponse);
            if (bDecoderResponse && MFX_ERR_NONE == mfx_res)
            {
                m_DecoderResponse.response = *pResponse;
            }
        }
        if ((MFX_ERR_NONE == mfx_res) && bDecoderResponse) ++(m_DecoderResponse.refcount);
#endif
    }

    return mfx_res;
}

mfxStatus MfxOmxFrameAllocator::FreeFrames(mfxFrameAllocResponse *pResponse)
{
    if (!pResponse) return MFX_ERR_NULL_PTR;

    if (!memcmp(pResponse, &(m_DecoderResponse.response), sizeof(mfxFrameAllocResponse)))
    {
        if (!m_DecoderResponse.refcount) return MFX_ERR_UNKNOWN; // should not occur, just in case

        --(m_DecoderResponse.refcount);
        if (m_DecoderResponse.refcount) return MFX_ERR_NONE;

        MFX_OMX_ZERO_MEMORY(m_DecoderResponse);
    }
#ifdef OMX_ENABLE_DECVPP
    if (!memcmp(pResponse, &(m_VPPResponse.response), sizeof(mfxFrameAllocResponse)))
    {
        if (!m_VPPResponse.refcount) return MFX_ERR_UNKNOWN; // should not occur, just in case

        --(m_VPPResponse.refcount);
        if (m_VPPResponse.refcount) return MFX_ERR_NONE;

        MFX_OMX_ZERO_MEMORY(m_VPPResponse);
    }
#endif
    return ReleaseResponse(pResponse);
}

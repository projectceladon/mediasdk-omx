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

#include "mfx_omx_bst_pool.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_bst_pool"

/*------------------------------------------------------------------------------*/

MfxOmxBitstreamsPool::MfxOmxBitstreamsPool(mfxU32 CodecId, mfxStatus &sts):
    MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxBitstream>(sts),
    m_codecId(CodecId),
    m_bCodecDataSent(false),
    m_bRemoveSPSPPS(true),
    m_dbg_file(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}


/*------------------------------------------------------------------------------*/

MfxOmxBitstreamsPool::~MfxOmxBitstreamsPool(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstreamsPool::PrepareBitstream(
    OMX_BUFFERHEADERTYPE* pBuffer,
    mfxInfoMFX* pMfxInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pAddBufInfo = NULL;

    if (!pBuffer || !pMfxInfo) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pAddBufInfo = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;
        if (!pAddBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        memset(&(pAddBufInfo->sBitstream), 0, sizeof(mfxBitstream));
        {
            pAddBufInfo->sBitstream.Data = (mfxU8*)pBuffer->pBuffer;
            pAddBufInfo->sBitstream.MaxLength = pBuffer->nAllocLen;
        }
        mfx_omx_reset_bitstream(&(pAddBufInfo->sBitstream));
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}


/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstreamsPool::UseBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = LoadBitstream(pBuffer);
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxBitstream>::UseBuffer(pBuffer);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstreamsPool::IsBufferLocked(
    OMX_BUFFERHEADERTYPE* pBuffer,
    bool& bIsLocked)
{
    bIsLocked = false;
    return (pBuffer)? MFX_ERR_NONE: MFX_ERR_NULL_PTR;
}

/*------------------------------------------------------------------------------*/
void MfxOmxBitstreamsPool::SetRemovingSpsPps(bool bRemove)
{
    m_bRemoveSPSPPS = bRemove;
}
/*------------------------------------------------------------------------------*/

bool MfxOmxBitstreamsPool::IsRemovingSPSPPSNeeded()
{
   return m_bRemoveSPSPPS;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstreamsPool::LoadBitstream(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pAddBufInfo = MfxOmxGetOutputBufferInfo(pBuffer);

    if (pBuffer && pAddBufInfo)
    {
        mfx_omx_reset_bitstream(&(pAddBufInfo->sBitstream));
    }
    else mfx_res = MFX_ERR_NULL_PTR;
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstreamsPool::RemoveHeaders(mfxBitstream *Bitstream)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfxU8 * data = Bitstream->Data + Bitstream->DataOffset;

    mfxU8 * begin = Bitstream->Data + Bitstream->DataOffset;
    mfxU8 * end = begin + Bitstream->DataLength;

    bool shortStartCode = false;

    for (NalUnit nalu = GetNalUnit(begin, end); nalu != NalUnit(); nalu = GetNalUnit(begin, end))
    {
        mfxU8 type = (MFX_CODEC_HEVC == m_codecId) ?
                    (nalu.begin[nalu.numZero+1] & 0x7E) >> 1 :
                     nalu.begin[nalu.numZero+1] & 0x1F;
        if (type == 1 || type == 5 || type == 19)
        {
            mfxU32 skip = nalu.begin - data;
            Bitstream->DataOffset += skip;
            Bitstream->DataLength -= skip;

            if (nalu.numZero == 2) shortStartCode = true;
            break;
        }
        begin = nalu.end;
    }

    if (Bitstream->DataOffset == 0 && shortStartCode)
    {
        memmove(Bitstream->Data + 1, Bitstream->Data, Bitstream->DataLength);
        Bitstream->Data[0] = 0;
        Bitstream->DataLength ++;
    }
    else if (Bitstream->DataOffset > 0)
    {
        /** Non zero offsets are wrongly handled by Android infrastructures
        *  and some CTS tests. An example of wrong handling is "testMultipleVirtualDisplays".
        */
        if (shortStartCode)
        {
            memmove(Bitstream->Data + 1, Bitstream->Data + Bitstream->DataOffset, Bitstream->DataLength);
            Bitstream->Data[0] = 0;
            Bitstream->DataLength ++;
        }
        else
        {
            memmove(Bitstream->Data, Bitstream->Data + Bitstream->DataOffset, Bitstream->DataLength);
        }
        Bitstream->DataOffset = 0;
    }

    return mfx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxBitstreamsPool::SendBitstream(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxBufferInfo* pAddBufInfo = MfxOmxGetOutputBufferInfo(pBuffer);

    if (pBuffer && pAddBufInfo)
    {
        // synchronizing parameters
        if (!(pAddBufInfo->pSyncPoint) && (MFX_BITSTREAM_SPSPPS != pAddBufInfo->sBitstream.DataFlag))
        { // that's special frame indicating EOS
            pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        else
        {
            pBuffer->nTimeStamp = MFX2OMX_TIME(pAddBufInfo->sBitstream.TimeStamp);
            pBuffer->nFlags = 0;
            {
                pBuffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;

                if (MFX_CODEC_AVC == m_codecId ||
                    MFX_CODEC_HEVC == m_codecId)
                {
                    if (m_bCodecDataSent)
                    {
                        if (m_bRemoveSPSPPS) RemoveHeaders(&pAddBufInfo->sBitstream);
                    }
                    else
                    {
                        // the first output buffer
                        pBuffer->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
                        m_bCodecDataSent = true;
                    }
                }

                pBuffer->nFilledLen = pAddBufInfo->sBitstream.DataLength;
                pBuffer->nOffset = pAddBufInfo->sBitstream.DataOffset;
            }

            if (pAddBufInfo->sBitstream.FrameType & MFX_FRAMETYPE_IDR)
            {
                pBuffer->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
            }

            MFX_OMX_AUTO_TRACE_I64(pBuffer->nTimeStamp);
            MFX_OMX_AUTO_TRACE_I32(pBuffer->nFilledLen);
            mfx_omx_dump(pAddBufInfo->sBitstream.Data + pAddBufInfo->sBitstream.DataOffset,
                1, pAddBufInfo->sBitstream.DataLength, m_dbg_file);
        }
        // releasing sample to be used by downstream plug-in
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
}

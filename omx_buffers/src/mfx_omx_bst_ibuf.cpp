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

#include "mfx_omx_bst_ibuf.h"
#include "mfx_omx_avc_bitstream.h"
#include "mfx_omx_avc_nal_spl.h"

#ifdef ENABLE_READ_SEI
#include "mfx_omx_hevc_bitstream.h"
#endif

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_bst_ibuf"

/*------------------------------------------------------------------------------*/

#ifdef ENABLE_READ_SEI
// possible markers of coded sloces NAL UNITs
const std::vector<mfxU32> MfxOmxHEVCFrameConstructor::NAL_UT_CODED_SLICEs = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 17, 18, 19, 20, 21 };
#endif

/*------------------------------------------------------------------------------*/

static inline mfxU32 BstGetU32(mfxU8* pBuf)
{
    if (!pBuf) return 0;
    return ((*pBuf) << 24) | (*(pBuf + 1) << 16) | (*(pBuf + 2) << 8) | (*(pBuf + 3));
}

/*------------------------------------------------------------------------------*/

static inline void BstSet64(mfxU64 nValue, mfxU8* pBuf)
{
    if (pBuf)
    {
        mfxU32 i = 0;
        for (i = 0; i < 8; ++i)
        {
            *pBuf = (mfxU8)(nValue >> (8 * i));
            ++pBuf;
        }
    }
    return;
}

static inline void BstSet32(mfxU32 nValue, mfxU8* pBuf)
{
    if (pBuf)
    {
        mfxU32 i = 0;
        for (i = 0; i < 4; ++i)
        {
            *pBuf = (mfxU8)(nValue >> (8 * i));
            ++pBuf;
        }
    }
    return;
}

/*------------------------------------------------------------------------------*/

static inline void BstSet16(mfxU16 nValue, mfxU8* pBuf)
{
    if (pBuf)
    {
        for (mfxU32 i = 0; i < 2; ++i)
        {
            *pBuf = (mfxU8)(nValue >> (8 * i));
            ++pBuf;
        }
    }
    return;
}

/*------------------------------------------------------------------------------*/

MfxOmxFrameConstructor::MfxOmxFrameConstructor(mfxStatus &sts):
    m_bs_state(MfxOmxBS_HeaderAwaiting),
    m_profile(MFX_PROFILE_UNKNOWN),
    m_pBst(NULL),
    m_bEOS(false),
    m_nBstBufReallocs(0),
    m_nBstBufCopyBytes(0),
    m_dbg_file(NULL),
    m_dbg_file_fc(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_ZERO_MEMORY(m_BstHeader);
    MFX_OMX_ZERO_MEMORY(m_BstBuf);
    MFX_OMX_ZERO_MEMORY(m_BstIn);
    MFX_OMX_ZERO_MEMORY(m_fr_info);

    sts = MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

MfxOmxFrameConstructor::~MfxOmxFrameConstructor(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (m_BstBuf.Data)
    {
        MFX_OMX_AUTO_TRACE_I32(m_BstBuf.MaxLength);
        MFX_OMX_AUTO_TRACE_I32(m_nBstBufReallocs);
        MFX_OMX_AUTO_TRACE_I32(m_nBstBufCopyBytes);

        MFX_OMX_FREE(m_BstBuf.Data);
    }

    MFX_OMX_FREE(m_BstHeader.Data);
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::Init(
    mfxU16 profile,
    mfxFrameInfo fr_info )
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_profile = profile;
    m_fr_info = fr_info;
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::LoadHeader(mfxU8* data, mfxU32 size, bool header)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_P(data);
    MFX_OMX_AUTO_TRACE_I32(size);
    if (!data || !size) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        if (header)
        {
            // if new header arrived after reset we are ignoring previously collected header data
            if (m_bs_state == MfxOmxBS_Resetting)
            {
                m_bs_state = MfxOmxBS_HeaderObtained;
            }
            else if (size)
            {
                mfxU32 needed_MaxLength = 0;
                mfxU8* new_data = NULL;

                needed_MaxLength = m_BstHeader.DataOffset + m_BstHeader.DataLength + size; // offset should be 0
                if (m_BstHeader.MaxLength < needed_MaxLength)
                { // increasing buffer capacity if needed
                    new_data = (mfxU8*)realloc(m_BstHeader.Data, needed_MaxLength);
                    if (new_data)
                    {
                        // setting new values
                        m_BstHeader.Data = new_data;
                        m_BstHeader.MaxLength = needed_MaxLength;
                    }
                    else mfx_res = MFX_ERR_MEMORY_ALLOC;
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    std::copy(data, data + size, m_BstHeader.Data + m_BstHeader.DataOffset + m_BstHeader.DataLength);
                    m_BstHeader.DataLength += size;
                }
                if (MfxOmxBS_HeaderAwaiting == m_bs_state) m_bs_state = MfxOmxBS_HeaderCollecting;
            }
        }
        else
        {
            // We have generic data. In case we are in Resetting state (i.e. seek mode)
            // we attach header to the bitstream, other wise we are moving in Obtained state.
            if (MfxOmxBS_HeaderCollecting == m_bs_state)
            {
                // As soon as we are receving first non header data we are stopping collecting header
                m_bs_state = MfxOmxBS_HeaderObtained;
            }
            else if (MfxOmxBS_Resetting == m_bs_state)
            {
                // if reset detected and we have header data buffered - we are going to load it
                mfx_res = BstBufRealloc(m_BstHeader.DataLength);
                if (MFX_ERR_NONE == mfx_res)
                {
                    std::copy(m_BstHeader.Data + m_BstHeader.DataOffset,
                              m_BstHeader.Data + m_BstHeader.DataOffset + m_BstHeader.DataLength,
                              m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength);
                    m_BstBuf.DataLength += m_BstHeader.DataLength;
                    m_nBstBufCopyBytes += m_BstHeader.DataLength;
                    mfx_omx_dump(m_BstHeader.Data + m_BstHeader.DataOffset, 1, m_BstHeader.DataLength, m_dbg_file_fc);
                }
                m_bs_state = MfxOmxBS_HeaderObtained;
            }
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::Load_None(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = LoadHeader(data, size, b_header);
    if ((MFX_ERR_NONE == mfx_res) && m_BstBuf.DataLength)
    {
        mfx_res = BstBufRealloc(size);
        if (MFX_ERR_NONE == mfx_res)
        {
            std::copy(data, data + size, m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength);
            m_BstBuf.DataLength += size;
            m_nBstBufCopyBytes += size;
        }
        // data copied - sample can be released
        ReleaseSample();
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        if (m_BstBuf.DataLength) m_pBst = &m_BstBuf;
        else
        {
            m_BstIn.Data = data;
            m_BstIn.DataOffset = 0;
            m_BstIn.DataLength = size;
            m_BstIn.MaxLength = size;
            if (bCompleteFrame)
                m_BstIn.DataFlag |= MFX_BITSTREAM_COMPLETE_FRAME;

            m_pBst = &m_BstIn;
        }
        m_pBst->TimeStamp = pts;
    }
    else m_pBst = NULL;
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_P(data);
    MFX_OMX_AUTO_TRACE_I32(size);
    MFX_OMX_AUTO_TRACE_I64(pts);
    if (!data || !size) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_omx_dump(data, 1, size, m_dbg_file);

        Load_None(data, size, pts, b_header, bCompleteFrame);
    }
    MFX_OMX_AT__mfxBitstream(m_BstBuf);
    MFX_OMX_AT__mfxBitstream(m_BstIn);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::Unload(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = BstBufSync();

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

// NOTE: we suppose that Load/Unload were finished
mfxStatus MfxOmxFrameConstructor::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU8* data = NULL;
    mfxU32 allocated_length = 0;

    // saving allocating information about internal buffer
    data = m_BstBuf.Data;
    allocated_length = m_BstBuf.MaxLength;

    // resetting frame constructor
    m_pBst = NULL;
    m_bEOS = false;
    MFX_OMX_ZERO_MEMORY(m_BstBuf);
    MFX_OMX_ZERO_MEMORY(m_BstIn);

    // restoring allocating information about internal buffer
    m_BstBuf.Data = data;
    m_BstBuf.MaxLength = allocated_length;

    // we have some header data and will attempt to return it
    if (m_bs_state >= MfxOmxBS_HeaderCollecting) m_bs_state = MfxOmxBS_Resetting;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::BstBufRealloc(mfxU32 add_size)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU32 needed_MaxLength = 0;
    mfxU8* new_data = NULL;

    if (add_size)
    {
        needed_MaxLength = m_BstBuf.DataOffset + m_BstBuf.DataLength + add_size; // offset should be 0
        if (m_BstBuf.MaxLength < needed_MaxLength)
        { // increasing buffer capacity if needed
            new_data = (mfxU8*)realloc(m_BstBuf.Data, needed_MaxLength);
            if (new_data)
            {
                // collecting statistics
                ++m_nBstBufReallocs;
                if (new_data != m_BstBuf.Data) m_nBstBufCopyBytes += m_BstBuf.MaxLength;
                // setting new values
                m_BstBuf.Data = new_data;
                m_BstBuf.MaxLength = needed_MaxLength;
            }
            else mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::BstBufMalloc(mfxU32 new_size)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU32 needed_MaxLength = 0;

    if (new_size)
    {
        needed_MaxLength = new_size;
        if (m_BstBuf.MaxLength < needed_MaxLength)
        { // increasing buffer capacity if needed
            MFX_OMX_FREE(m_BstBuf.Data);
            m_BstBuf.Data = (mfxU8*)malloc(needed_MaxLength);
            m_BstBuf.MaxLength = needed_MaxLength;
            ++m_nBstBufReallocs;
        }
        if (!(m_BstBuf.Data))
        {
            m_BstBuf.MaxLength = 0;
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxFrameConstructor::BstBufSync(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (m_pBst)
    {
        if (m_pBst == &m_BstBuf)
        {
            if (m_BstBuf.DataLength && m_BstBuf.DataOffset)
            { // shifting data to the beginning of the buffer
                memmove(m_BstBuf.Data, m_BstBuf.Data + m_BstBuf.DataOffset, m_BstBuf.DataLength);
                m_nBstBufCopyBytes += m_BstBuf.DataLength;
            }
            m_BstBuf.DataOffset = 0;
        }
        if ((m_pBst == &m_BstIn) && m_BstIn.DataLength)
        { // copying data from m_BstIn to m_pBstBuf
            // Note: we read data from m_BstIn, thus here m_pBstBuf is empty
            mfx_res = BstBufMalloc(m_BstIn.DataLength);
            if (MFX_ERR_NONE == mfx_res)
            {
                std::copy(m_BstIn.Data + m_BstIn.DataOffset,
                          m_BstIn.Data + m_BstIn.DataOffset + m_BstIn.DataLength,
                          m_BstBuf.Data);
                m_BstBuf.DataOffset = 0;
                m_BstBuf.DataLength = m_BstIn.DataLength;
                m_BstBuf.TimeStamp  = m_BstIn.TimeStamp;
                m_BstBuf.DataFlag   = m_BstIn.DataFlag;
                m_nBstBufCopyBytes += m_BstIn.DataLength;
            }
            MFX_OMX_ZERO_MEMORY(m_BstIn);
        }
        m_pBst = NULL;
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxBitstream* MfxOmxFrameConstructor::GetMfxBitstream(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxBitstream* pBst = NULL;

    if (m_BstBuf.Data && m_BstBuf.DataLength)
    {
        pBst = &m_BstBuf;
    }
    else if (m_BstIn.Data && m_BstIn.DataLength)
    {
        pBst = &m_BstIn;
    }
    else
    {
        pBst = &m_BstBuf;
    }

    MFX_OMX_AUTO_TRACE_P(&m_BstIn);
    MFX_OMX_AUTO_TRACE_P(&m_BstBuf);
    MFX_OMX_AUTO_TRACE_P(pBst);
    return pBst;
}

/*------------------------------------------------------------------------------*/

MfxOmxAVCFrameConstructor::MfxOmxAVCFrameConstructor(mfxStatus &sts):
    MfxOmxFrameConstructor(sts)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_ZERO_MEMORY(m_SPS);
    MFX_OMX_ZERO_MEMORY(m_PPS);
}

/*------------------------------------------------------------------------------*/

MfxOmxAVCFrameConstructor::~MfxOmxAVCFrameConstructor(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_FREE(m_SPS.Data);
    MFX_OMX_FREE(m_PPS.Data);
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAVCFrameConstructor::SaveHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS, bool isReset)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (isReset) m_bs_state = MfxOmxBS_Resetting;

    if (pSPS)
    {
        if (m_SPS.MaxLength < pSPS->DataLength)
        {
            m_SPS.Data = (mfxU8*)realloc(m_SPS.Data, pSPS->DataLength);
            if (!m_SPS.Data)
                return MFX_ERR_MEMORY_ALLOC;
            m_SPS.MaxLength = pSPS->DataLength;
        }
        std::copy(pSPS->Data + pSPS->DataOffset,
                  pSPS->Data + pSPS->DataOffset + pSPS->DataLength,
                  m_SPS.Data);
        m_SPS.DataLength = pSPS->DataLength;
    }
    if (pPPS)
    {
        if (m_PPS.MaxLength < pPPS->DataLength)
        {
            m_PPS.Data = (mfxU8*)realloc(m_PPS.Data, pPPS->DataLength);
            if (!m_PPS.Data)
                return MFX_ERR_MEMORY_ALLOC;
            m_PPS.MaxLength = pPPS->DataLength;
        }
        std::copy(pPPS->Data + pPPS->DataOffset,
                  pPPS->Data + pPPS->DataOffset + pPPS->DataLength,
                  m_PPS.Data);
        m_PPS.DataLength = pPPS->DataLength;
    }
    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAVCFrameConstructor::FindHeaders(mfxU8* data, mfxU32 size, bool &bFoundSps, bool &bFoundPps, bool &bFoundSei)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    bFoundSps = false;
    bFoundPps = false;
    bFoundSei = false;

    if (data && size)
    {
        mfxI32 startCodeSize;
        mfxI32 code;
        mfxU32 length;
        for (; size > 3;)
        {
            startCodeSize = 0;
            code = FindStartCode(data, size, startCodeSize);
            if (isSPS(code))
            {
                mfxBitstream sps;
                MFX_OMX_ZERO_MEMORY(sps);
                sps.Data = data - startCodeSize;

                length = size + startCodeSize;
                code = FindStartCode(data, size, startCodeSize);
                if (-1 != code)
                    length -= size + startCodeSize;
                sps.DataLength = length;
                MFX_OMX_LOG("Found SPS size %d", length);
                mfx_res = SaveHeaders(&sps, NULL, false);
                if (MFX_ERR_NONE != mfx_res) return mfx_res;
                bFoundSps = true;
            }
            if (isPPS(code))
            {
                mfxBitstream pps;
                MFX_OMX_ZERO_MEMORY(pps);
                pps.Data = data - startCodeSize;

                length = size + startCodeSize;
                code = FindStartCode(data, size, startCodeSize);
                if (-1 != code)
                    length -= size + startCodeSize;
                pps.DataLength = length;
                MFX_OMX_LOG("Found PPS size %d", length);
                mfx_res = SaveHeaders(NULL, &pps, false);
                if (MFX_ERR_NONE != mfx_res) return mfx_res;
                bFoundPps = true;
            }
#ifdef ENABLE_READ_SEI
            while (isSEI(code))
            {
                mfxBitstream sei = {};
                MFX_OMX_ZERO_MEMORY(sei);
                sei.Data = data - startCodeSize;
                sei.DataLength = size + startCodeSize;
                code = FindStartCode(data, size, startCodeSize);
                if (-1 != code)
                    sei.DataLength -= size + startCodeSize;
                MFX_OMX_LOG("Found SEI size %d", sei.DataLength);
                mfx_res = SaveSEI(&sei);
                if (MFX_ERR_NONE != mfx_res) return mfx_res;
                bFoundSei = true;
            }
            // start code == coded slice, so no need wait SEI
            if (!IsNeedWaitSEI(code)) bFoundSei = true;
#endif
            if (-1 == code) break;
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAVCFrameConstructor::LoadHeader(mfxU8* data, mfxU32 size, bool header)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    bool bFoundSps = false;
    bool bFoundPps = false;
    bool bFoundSei = false;

    if (header && data && size)
    {
        if (MfxOmxBS_HeaderAwaiting == m_bs_state) m_bs_state = MfxOmxBS_HeaderCollecting;

        mfx_res = FindHeaders(data, size, bFoundSps, bFoundPps, bFoundSei);
        if (MFX_ERR_NONE == mfx_res && bFoundSps && bFoundPps)
        {
#ifdef ENABLE_READ_SEI
            m_bs_state = bFoundSei ? MfxOmxBS_HeaderObtained : MfxOmxBS_HeaderWaitSei;
#else
            m_bs_state = MfxOmxBS_HeaderObtained;
#endif
        }
    }
    else if (MfxOmxBS_Resetting == m_bs_state)
    {
        mfx_res = FindHeaders(data, size, bFoundSps, bFoundPps, bFoundSei);
        if (MFX_ERR_NONE == mfx_res)
        {
            if (!bFoundSps || !bFoundPps)
            {
                // In case we are in Resetting state (i.e. seek mode)
                // and bitstream has no headers, we attach header to the bitstream.
                mfx_res = BstBufRealloc(m_SPS.DataLength + m_PPS.DataLength);
                if (MFX_ERR_NONE == mfx_res)
                {
                    std::copy(m_SPS.Data, m_SPS.Data + m_SPS.DataLength, m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength);
                    m_BstBuf.DataLength += m_SPS.DataLength;

                    std::copy(m_PPS.Data, m_PPS.Data + m_PPS.DataLength, m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength);
                    m_BstBuf.DataLength += m_PPS.DataLength;

                    m_nBstBufCopyBytes += m_SPS.DataLength + m_PPS.DataLength;
                    mfx_omx_dump(m_SPS.Data, 1, m_SPS.DataLength, m_dbg_file_fc);
                    mfx_omx_dump(m_PPS.Data, 1, m_PPS.DataLength, m_dbg_file_fc);
                }
            }
            m_bs_state = MfxOmxBS_HeaderObtained;
        }
    }
    else if (MfxOmxBS_HeaderCollecting == m_bs_state)
    {
        // As soon as we are receving first non header data we are stopping collecting header
        m_bs_state = MfxOmxBS_HeaderObtained;
    }
#ifdef ENABLE_READ_SEI
    else if (MfxOmxBS_HeaderWaitSei == m_bs_state)
    {
        mfx_res = FindHeaders(data, size, bFoundSps, bFoundPps, bFoundSei);
        if (MFX_ERR_NONE == mfx_res && bFoundSps && bFoundPps)
        {
            m_bs_state = bFoundSei ? MfxOmxBS_HeaderObtained : MfxOmxBS_HeaderWaitSei;
        }
    }
#endif
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxI32 MfxOmxAVCFrameConstructor::FindStartCode(mfxU8 * (&pb), mfxU32 & size, mfxI32 & startCodeSize)
{
    mfxU32 zeroCount = 0;
    static const mfxU8 nalUnitTypeBits = 0x1f;

    mfxI32 i = 0;
    for (; i < (mfxI32)size - 2; )
    {
        if (pb[1])
        {
            pb += 2;
            i += 2;
            continue;
        }

        zeroCount = 0;
        if (!pb[0]) zeroCount++;

        mfxU32 j;
        for (j = 1; j < (mfxU32)size - i; j++)
        {
            if (pb[j]) break;
        }

        zeroCount = zeroCount ? j: j - 1;

        pb += j;
        i += j;

        if (i >= (mfxI32)size) break;

        if (zeroCount >= 2 && pb[0] == 1)
        {
            startCodeSize = MFX_OMX_MIN(zeroCount + 1, 4);
            size -= i + 1;
            pb++; // remove 0x01 symbol
            zeroCount = 0;
            if (size >= 1) return pb[0] & nalUnitTypeBits;
            else
            {
                pb -= startCodeSize;
                size += startCodeSize;
                startCodeSize = 0;
                return -1;
            }
        }
        zeroCount = 0;
    }

    if (!zeroCount)
    {
        for (mfxU32 k = 0; k < size - i; k++, pb++)
        {
            if (pb[0])
            {
                zeroCount = 0;
                continue;
            }
            zeroCount++;
        }
    }

    zeroCount = MFX_OMX_MIN(zeroCount, 3);
    pb -= zeroCount;
    size = zeroCount;
    zeroCount = 0;
    startCodeSize = 0;
    return -1;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAVCFrameConstructor::Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = MfxOmxFrameConstructor::Load(data, size, pts, b_header, bCompleteFrame);
    mfx_omx_dump(data, 1, size, m_dbg_file_fc);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxAVCFrameConstructor::IsSetInterlaceFlag(bool * bInterlaced)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_P(m_SPS.Data);

    if (NULL == m_SPS.Data)
    {
        MFX_OMX_AUTO_TRACE_MSG("ERROR: Not found SPS");
        return false;
    }

    MFX_OMX_AUTO_TRACE_MSG("Calling BytesSwapper::SwapMemory()");

    std::vector<mfxU8> swappingMemory;
    mfxU32 swappingMemotySize = m_SPS.DataLength - 5; // size SPS in bytes without start code and marker
    swappingMemory.resize(swappingMemotySize + 8);
    AVCParser::BytesSwapper::SwapMemory(&(swappingMemory[0]), swappingMemotySize, (m_SPS.Data + 5), swappingMemotySize);

    MFX_OMX_AUTO_TRACE_MSG("Calling AVCHeadersBitstream.Reset()");
    MFX_OMX_AUTO_TRACE_U32(swappingMemotySize);

    AVCParser::AVCHeadersBitstream bitStream;
    bitStream.Reset(&(swappingMemory[0]), swappingMemotySize);

    AVCParser::AVCSeqParamSet sps;
    if (MFX_ERR_NONE == bitStream.GetSequenceParamSet(&sps))
    {
        *bInterlaced = !sps.frame_mbs_only_flag;
        return true;
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("ERROR: Invalid SPS");
        return false;
    }
    return false;
}

/*------------------------------------------------------------------------------*/

mfxI32 MfxOmxHEVCFrameConstructor::FindStartCode(mfxU8 * (&pb), mfxU32 & size, mfxI32 & startCodeSize)
{
    mfxU32 zeroCount = 0;
    static const mfxU8 NAL_UNITTYPE_BITS_H265 = 0x7e;
    static const mfxU8 NAL_UNITTYPE_SHIFT_H265 = 1;

    mfxI32 i = 0;
    for (; i < (mfxI32)size - 2; )
    {
        if (pb[1])
        {
            pb += 2;
            i += 2;
            continue;
        }

        zeroCount = 0;
        if (!pb[0])
            zeroCount++;

        mfxU32 j;
        for (j = 1; j < (mfxU32)size - i; j++)
        {
            if (pb[j])
                break;
        }

        zeroCount = zeroCount ? j: j - 1;

        pb += j;
        i += j;

        if (i >= (mfxI32)size)
        {
            break;
        }

        if (zeroCount >= 2 && pb[0] == 1)
        {
            startCodeSize = MFX_OMX_MIN(zeroCount + 1, 4);
            size -= i + 1;
            pb++; // remove 0x01 symbol
            if (size >= 1)
            {
                return (pb[0] & NAL_UNITTYPE_BITS_H265) >> NAL_UNITTYPE_SHIFT_H265;
            }
            else
            {
                pb -= startCodeSize;
                size += startCodeSize;
                startCodeSize = 0;
                return -1;
            }
        }

        zeroCount = 0;
    }

    if (!zeroCount)
    {
        for (mfxU32 k = 0; k < size - i; k++, pb++)
        {
            if (pb[0])
            {
                zeroCount = 0;
                continue;
            }

            zeroCount++;
        }
    }

    zeroCount = MFX_OMX_MIN(zeroCount, 3);
    pb -= zeroCount;
    size = zeroCount;
    startCodeSize = zeroCount;
    return -1;
}

/*------------------------------------------------------------------------------*/

#ifdef ENABLE_READ_SEI
mfxStatus MfxOmxHEVCFrameConstructor::SaveSEI(mfxBitstream *pSEI)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (nullptr != pSEI && nullptr != pSEI->Data)
    {
        std::vector<mfxU8> swappingMemory;
        mfxU32 swappingMemorySize = pSEI->DataLength - 5;
        swappingMemory.resize(swappingMemorySize + 8);

        std::vector<mfxU32> SEINames = {SEI_MASTERING_DISPLAY_COLOUR_VOLUME, SEI_CONTENT_LIGHT_LEVEL_INFO};
        for (auto const& sei_name : SEINames) // look for sei
        {
            mfxPayload sei = {};
            sei.BufSize = pSEI->DataLength;
            sei.Data = (mfxU8*)realloc(sei.Data, pSEI->DataLength);
            if (nullptr == sei.Data)
            {
                MFX_OMX_AUTO_TRACE_MSG("ERROR: SEI was not alloacated");
                return MFX_ERR_MEMORY_ALLOC;
            }

            MFX_OMX_AUTO_TRACE_MSG("Calling ByteSwapper::SwapMemory()");

            AVCParser::BytesSwapper::SwapMemory(&(swappingMemory[0]), swappingMemorySize, (pSEI->Data + 5), swappingMemorySize);

            MFX_OMX_AUTO_TRACE_MSG("Calling HEVCHeadersBitstream.Reset()");
            MFX_OMX_AUTO_TRACE_U32(swappingMemorySize);

            HEVCParser::HEVCHeadersBitstream bitStream;
            bitStream.Reset(&(swappingMemory[0]), swappingMemorySize);

            MFX_OMX_AUTO_TRACE_MSG("Calling HEVCHeadersBitstream.GetSEI() for SEI");
            MFX_OMX_AUTO_TRACE_U32(sei_name);

            MFX_OMX_TRY_AND_CATCH(
                bitStream.GetSEI(&sei, sei_name),
                sei.NumBit = 0);
            if (sei.Type == sei_name && sei.NumBit > 0)
            {
                // replace sei
                auto old_sei = SEIMap.find(sei_name);
                if (old_sei != SEIMap.end())
                {
                    MFX_OMX_FREE(old_sei->second.Data);
                    SEIMap.erase(old_sei);
                }
                SEIMap.insert(std::pair<mfxU32, mfxPayload>(sei_name, sei));
            }
            else
                MFX_OMX_FREE(sei.Data);
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxPayload* MfxOmxHEVCFrameConstructor::GetSei(mfxU32 type)
{
    auto sei = SEIMap.find(type);
    if (sei != SEIMap.end())
        return &(sei->second);

    return nullptr;
}
#endif

/*------------------------------------------------------------------------------*/

MfxOmxVC1FrameConstructor::MfxOmxVC1FrameConstructor(mfxStatus &sts):
    MfxOmxFrameConstructor(sts)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

MfxOmxVC1FrameConstructor::~MfxOmxVC1FrameConstructor(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVC1FrameConstructor::Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU32 startcode = 0;
    mfxU32 addSize = 0;

    if (b_header)
    {
        bool bIsAdvanced = false;
        mfxU32 i = 0, remained_size = size;

        // if profile is advanced, we should find sequence header with 0x0000010f start code
        for (i = 0; i < size; ++i, --remained_size)
        {
            if (remained_size < 4) break;
            if ((0 == data[i]) && (0 == data[i+1]) && (1 == data[i+2]) && (0xf == data[i+3]))
            {
                // what we have before sequence header we do not need and should drop
                data += i;
                size -= i;
                bIsAdvanced = true;
                break;
            }
        }
        if (bIsAdvanced)
        {
            MFX_OMX_AUTO_TRACE_MSG("MFX_PROFILE_VC1_ADVANCED");
            m_profile = MFX_PROFILE_VC1_ADVANCED;

            // reconstructing header for advanced profile
            if ((MFX_ERR_NONE == mfx_res))
            {
                mfx_res = BstBufRealloc(size);
            }
            if (MFX_ERR_NONE == mfx_res)
            {
                std::copy(data, data + size, m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength);
                m_BstBuf.DataLength += size;
                m_nBstBufCopyBytes += size;
            }
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("MFX_PROFILE_VC1_MAIN or MFX_PROFILE_VC1_SIMPLE");
            m_profile = MFX_PROFILE_VC1_MAIN; // or SIMPLE - for FC this does not matter

            // reconstructing header for sample & main profile
            mfx_res = BstBufRealloc(size + 20);
            if (MFX_ERR_NONE == mfx_res)
            {
                mfxU8* buf = m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength;

                BstSet32(0xC5000000, buf);
                BstSet32(size, buf + 4);

                std::copy(data, data + size, buf + 8);

                BstSet32(m_fr_info.CropH, buf + 8 + size);
                BstSet32(m_fr_info.CropW, buf + 8 + size + 4);
                BstSet32(0, buf + 8 + size + 8);

                m_BstBuf.DataLength += 8 + size + 12;
                m_nBstBufCopyBytes += 8 + size + 12;
            }
        }
        mfx_omx_dump(m_BstBuf.Data + m_BstBuf.DataOffset, 1, m_BstBuf.DataLength, m_dbg_file_fc);
        mfx_res = LoadHeader(m_BstBuf.Data + m_BstBuf.DataOffset, m_BstBuf.DataLength, b_header);
        ReleaseSample();
    }
    else
    { // Reconstruct general frames
        startcode = BstGetU32(data);
        switch (startcode)
        {
        case 0x010A: case 0x010C: case 0x010D: case 0x010E: case 0x010F:
        case 0x011B: case 0x011C: case 0x011D: case 0x011E: case 0x011F:
            addSize = 0;
            break;
        default:
            if (MFX_PROFILE_VC1_ADVANCED == m_profile) addSize = 4;
            else addSize = 8; // SIMPLE and MAIN
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = LoadHeader(data, size, b_header);
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = BstBufRealloc(size + addSize);

            if (MFX_ERR_NONE == mfx_res)
            {
                mfxU8* buf = m_BstBuf.Data + m_BstBuf.DataOffset + m_BstBuf.DataLength;

                if ( 4 == addSize ) BstSet32(0x0D010000, buf);
                else if ( 8 == addSize )
                {
                    BstSet32(size, buf);
                    BstSet32(0x00000000, buf + 4);
                }

                std::copy(data, data + size, buf + addSize);
                mfx_omx_dump(buf, 1, size + addSize, m_dbg_file_fc);

                m_BstBuf.DataLength += size + addSize;
                m_nBstBufCopyBytes += size + addSize;
            }
            // data copied - sample can be released
            ReleaseSample();
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        if (m_BstBuf.DataLength) m_pBst = &m_BstBuf;
        else
        {
            m_BstIn.Data = data;
            m_BstIn.DataOffset = 0;
            m_BstIn.DataLength = size;
            m_BstIn.MaxLength = size;
            if (bCompleteFrame)
                m_BstIn.DataFlag |= MFX_BITSTREAM_COMPLETE_FRAME;

            m_pBst = &m_BstIn;
        }
        m_pBst->TimeStamp = pts;
    }
    else m_pBst = NULL; // error

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

MfxOmxVP8FrameConstructor::MfxOmxVP8FrameConstructor(mfxStatus &sts):
    MfxOmxFrameConstructor(sts),
    m_bFirstSample(true)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

MfxOmxVP8FrameConstructor::~MfxOmxVP8FrameConstructor(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVP8FrameConstructor::Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = MfxOmxFrameConstructor::Load(data, size, pts, b_header, bCompleteFrame);

    if (m_bFirstSample)
    {
        mfxU8 IVFHeader[32] = { 0 };
        BstSet32(MFX_MAKEFOURCC('D','K','I','F'), IVFHeader);
        BstSet16(0, IVFHeader + 4); // version
        BstSet16(32, IVFHeader + 6); // length of header in bytes
        BstSet32(MFX_MAKEFOURCC('V','P','8','0'), IVFHeader + 8);
        BstSet16(1920, IVFHeader + 12); // width in pixels
        BstSet16(1088, IVFHeader + 14); // height in pixels
        BstSet32(30, IVFHeader + 16); // frame rate
        BstSet32(1, IVFHeader + 20); // time scale
        BstSet32(0, IVFHeader + 24); // number of frames in file

        mfx_omx_dump(IVFHeader, 1, sizeof(IVFHeader), m_dbg_file_fc);
        m_bFirstSample = false;
    }

    mfxU8 frameHeader[12] = { 0 };
    BstSet32(size, frameHeader);
    BstSet64(pts, frameHeader + 4);

    mfx_omx_dump(frameHeader, 1, sizeof(frameHeader), m_dbg_file_fc);
    mfx_omx_dump(data, 1, size, m_dbg_file_fc);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

MfxOmxVP9FrameConstructor::MfxOmxVP9FrameConstructor(mfxStatus &sts):
    MfxOmxFrameConstructor(sts),
    m_bFirstSample(true)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

MfxOmxVP9FrameConstructor::~MfxOmxVP9FrameConstructor(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVP9FrameConstructor::Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = MfxOmxFrameConstructor::Load(data, size, pts, b_header, bCompleteFrame);

    if (m_bFirstSample)
    {
        mfxU8 IVFHeader[32] = { 0 };
        BstSet32(MFX_MAKEFOURCC('D','K','I','F'), IVFHeader);
        BstSet16(0, IVFHeader + 4); // version
        BstSet16(32, IVFHeader + 6); // length of header in bytes
        BstSet32(MFX_MAKEFOURCC('V','P','9','0'), IVFHeader + 8);
        BstSet16(1920, IVFHeader + 12); // width in pixels
        BstSet16(1088, IVFHeader + 14); // height in pixels
        BstSet32(30, IVFHeader + 16); // frame rate
        BstSet32(1, IVFHeader + 20); // time scale
        BstSet32(0, IVFHeader + 24); // number of frames in file

        mfx_omx_dump(IVFHeader, 1, sizeof(IVFHeader), m_dbg_file_fc);
        m_bFirstSample = false;
    }

    mfxU8 frameHeader[12] = { 0 };
    BstSet32(size, frameHeader);
    BstSet64(pts, frameHeader + 4);

    mfx_omx_dump(frameHeader, 1, sizeof(frameHeader), m_dbg_file_fc);
    mfx_omx_dump(data, 1, size, m_dbg_file_fc);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

IMfxOmxFrameConstructor* MfxOmxFrameConstructorFactory::CreateFrameConstructor(MfxOmxFrameConstructorType fc_type, mfxStatus &sts)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    IMfxOmxFrameConstructor* fc = NULL;
    if (MfxOmxFC_AVC == fc_type)
    {
        MFX_OMX_NEW(fc, MfxOmxAVCFrameConstructor(sts));
        return fc;
    }
    else if (MfxOmxFC_HEVC == fc_type)
    {
        MFX_OMX_NEW(fc, MfxOmxHEVCFrameConstructor(sts));
        return fc;
    }
    else if (MfxOmxFC_VC1 == fc_type)
    {
        MFX_OMX_NEW(fc, MfxOmxVC1FrameConstructor(sts));
        return fc;
    }
    else if (MfxOmxFC_VP8 == fc_type)
    {
        MFX_OMX_NEW(fc, MfxOmxVP8FrameConstructor(sts));
        return fc;
    }
    else if (MfxOmxFC_VP9 == fc_type)
    {
        MFX_OMX_NEW(fc, MfxOmxVP9FrameConstructor(sts));
        return fc;
    }
    else
    {
        MFX_OMX_NEW(fc, MfxOmxFrameConstructor(sts));
        return fc;
    }
}

/*------------------------------------------------------------------------------*/

MfxOmxBitstream::MfxOmxBitstream(MfxOmxFrameConstructorType fc_type, mfxStatus &sts):
    MfxOmxInputBuffersPool<OMX_BUFFERHEADERTYPE>(sts),
    m_pBuffer(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (MFX_ERR_NONE == sts)
    {
        m_frameConstructor.reset(MfxOmxFrameConstructorFactory::CreateFrameConstructor(fc_type, sts));
    }
}

/*------------------------------------------------------------------------------*/

MfxOmxBitstream::~MfxOmxBitstream(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstream::InitFrameConstructor(
    mfxU16 profile,
    mfxFrameInfo fr_info )
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = m_frameConstructor->Init(profile, fr_info);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstream::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = m_frameConstructor->Reset();
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = MfxOmxInputBuffersPool<OMX_BUFFERHEADERTYPE>::Reset();
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstream::IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked)
{
    bIsLocked = false;
    return (pBuffer)? MFX_ERR_NONE: MFX_ERR_NULL_PTR;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstream::LoadOmxBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU8* pData = NULL;
    mfxU32 nFilledLen = 0;

    if (MFX_ERR_NONE == mfx_res)
    {
        if (pBuffer) m_pBuffer = pBuffer;
        else mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBuffer));
        MFX_OMX_AUTO_TRACE_I64(pBuffer->nTimeStamp);

        pData = pBuffer->pBuffer + pBuffer->nOffset;
        nFilledLen = pBuffer->nFilledLen;

        m_frameConstructor->SetEosMode(pBuffer->nFlags & OMX_BUFFERFLAG_EOS);

        mfx_res = m_frameConstructor->Load(
            pData, nFilledLen, OMX2MFX_TIME(pBuffer->nTimeStamp),
            (pBuffer->nFlags & OMX_BUFFERFLAG_CODECCONFIG),
            (pBuffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME));
    }
    if (MFX_ERR_NONE != mfx_res) Unload();
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxBitstream::Unload(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = m_frameConstructor->Unload();
    ReleaseSample();
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxBitstream::ReleaseSample(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), m_pBuffer, MFX_ERR_NONE);
}

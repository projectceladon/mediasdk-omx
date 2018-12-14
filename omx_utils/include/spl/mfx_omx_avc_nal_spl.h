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

#ifndef __MFX_OMX_AVC_NAL_SPL_H
#define __MFX_OMX_AVC_NAL_SPL_H

#include <vector>
#include "mfxstructures.h"

namespace AVCParser
{

class BytesSwapper
{
public:
    static void SwapMemory(mfxU8 *pDestination, mfxU32 &nDstSize, mfxU8 *pSource, mfxU32 nSrcSize);
};

class StartCodeIterator
{
public:

    StartCodeIterator();

    void Reset();

    mfxI32 Init(mfxBitstream * source);

    void SetSuggestedSize(mfxU32 size);

    mfxI32 CheckNalUnitType(mfxBitstream * source);

    mfxI32 GetNALUnit(mfxBitstream * source, mfxBitstream * destination);

    mfxI32 EndOfStream(mfxBitstream * destination);

private:
    mfxI32 FindStartCode(mfxU8 * (&pb), mfxU32 & size, mfxI32 & startCodeSize);

    std::vector<mfxU8>  m_prev;
    mfxU32   m_code;
    mfxU64   m_pts;

    mfxU8 * m_pSource;
    mfxU32  m_nSourceSize;

    mfxU8 * m_pSourceBase;
    mfxU32  m_nSourceBaseSize;

    mfxU32  m_suggestedSize;
};

class NALUnitSplitter
{
public:

    NALUnitSplitter();

    virtual ~NALUnitSplitter();

    virtual void Init();
    virtual void Release();

    virtual mfxI32 CheckNalUnitType(mfxBitstream * source);
    virtual mfxI32 GetNalUnits(mfxBitstream * source, mfxBitstream * &destination);

    virtual void Reset();

    virtual void SetSuggestedSize(mfxU32 size)
    {
        m_pStartCodeIter.SetSuggestedSize(size);
    }

protected:

    StartCodeIterator m_pStartCodeIter;

    mfxBitstream m_bitstream;
};

void SwapMemoryAndRemovePreventingBytes(mfxU8 *pDestination, mfxU32 &nDstSize, mfxU8 *pSource, mfxU32 nSrcSize);

} //namespace AVCParser

#endif // __MFX_OMX_AVC_NAL_SPL_H

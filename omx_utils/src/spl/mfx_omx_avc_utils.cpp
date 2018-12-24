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

#include <stdlib.h>
#include <assert.h>

#include "spl/mfx_omx_avc_utils.h"
#include "mfx_omx_utils.h"

OutputBitstream::OutputBitstream(mfxU8 * buf, size_t size, bool emulationControl)
: m_buf(buf)
, m_ptr(buf)
, m_bufEnd(buf + size)
, m_bitOff(0)
, m_emulationControl(emulationControl)
{
    if (m_ptr < m_bufEnd)
        *m_ptr = 0; // clear next byte
}

OutputBitstream::OutputBitstream(mfxU8 * buf, mfxU8 * bufEnd, bool emulationControl)
: m_buf(buf)
, m_ptr(buf)
, m_bufEnd(bufEnd)
, m_bitOff(0)
, m_emulationControl(emulationControl)
{
    if (m_ptr < m_bufEnd)
        *m_ptr = 0; // clear next byte
}

mfxU32 OutputBitstream::GetNumBits() const
{
    return mfxU32(8 * (m_ptr - m_buf) + m_bitOff);
}

void OutputBitstream::PutBit(mfxU32 bit)
{
    if (m_ptr >= m_bufEnd)
        throw EndOfBuffer();

    mfxU8 mask = mfxU8(0xff << (8 - m_bitOff));
    mfxU8 newBit = mfxU8(bit << (7 - m_bitOff));
    *m_ptr = (*m_ptr & mask) | newBit;

    if (++m_bitOff == 8)
    {
        if (m_emulationControl && m_ptr - 2 >= m_buf &&
            (*m_ptr & 0xfc) == 0 && *(m_ptr - 1) == 0 && *(m_ptr - 2) == 0)
        {
            if (m_ptr + 1 >= m_bufEnd)
                throw EndOfBuffer();

            *(m_ptr + 1) = *(m_ptr + 0);
            *(m_ptr + 0) = 0x03;
            m_ptr++;
        }

        m_bitOff = 0;
        m_ptr++;
        if (m_ptr < m_bufEnd)
            *m_ptr = 0; // clear next byte
    }
}

void OutputBitstream::PutBits(mfxU32 val, mfxU32 nbits)
{
    assert(nbits <= 32);

    for (; nbits > 0; nbits--)
        PutBit((val >> (nbits - 1)) & 1);
}

void OutputBitstream::PutUe(mfxU32 val)
{
    if (val == 0)
    {
        PutBit(1);
    }
    else
    {
        val++;
        mfxU32 nbits = 1;
        while (val >> nbits)
            nbits++;

        PutBits(0, nbits - 1);
        PutBits(val, nbits);
    }
}

void OutputBitstream::PutSe(mfxI32 val)
{
    (val <= 0)
        ? PutUe(-2 * val)
        : PutUe( 2 * val - 1);
}

void OutputBitstream::PutTrailingBits()
{
    PutBit(1);
    while (m_bitOff != 0)
        PutBit(0);
}

void OutputBitstream::PutRawBytes(mfxU8 const * begin, mfxU8 const * end)
{
    assert(m_bitOff == 0);

    if (m_bufEnd - m_ptr < end - begin)
        throw EndOfBuffer();

    std::copy(begin, end, m_ptr);
    m_bitOff = 0;
    m_ptr += end - begin;

    if (m_ptr < m_bufEnd)
        *m_ptr = 0;
}

void OutputBitstream::PutFillerBytes(mfxU8 filler, mfxU32 nbytes)
{
    assert(m_bitOff == 0);

    if (m_ptr + nbytes > m_bufEnd)
        throw EndOfBuffer();

    memset(m_ptr, filler, nbytes);
    m_ptr += nbytes;
}


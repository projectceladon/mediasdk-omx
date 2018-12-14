// Copyright (c) 2018 Intel Corporation
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

#ifdef HDR_SEI_PAYLOAD
#include "spl/mfx_omx_hevc_bitstream.h"

using namespace AVCParser;

namespace HEVCParser
{

#define hevcGetNBits( current_data, offset, nbits, data) \
    _avcGetBits(current_data, offset, nbits, data);

#define hevcNextBits(current_data, bp, nbits, data) \
    avcNextBits(current_data, bp, nbits, data)

void HEVCHeadersBitstream::GetSEI(mfxPayload *spl, mfxU32 type)
{
    if (nullptr == spl)
       return;
    mfxU32 code;
    mfxPayload currentSEI;

    hevcGetNBits(m_pbs, m_bitOffset, 8, code);
    while ((mfxI32)BytesLeft() > 0)
    {
        ParseSEI(&currentSEI);
        if (type == currentSEI.Type)
        {
            spl->Type = currentSEI.Type;
            spl->NumBit = currentSEI.NumBit;
            if ((currentSEI.NumBit / 8) > BytesLeft())
                throw AVC_exception(MFX_ERR_UNDEFINED_BEHAVIOR);

            if (nullptr != spl->Data)
            {
                for (mfxU32 i = 0; i < (spl->NumBit / 8); i++)
                {
                    hevcGetNBits(m_pbs, m_bitOffset, 8, spl->Data[i]);
                }
            }
            return;
        }
        else // skip SEI data
        {
            if ((currentSEI.NumBit / 8) > BytesLeft())
                throw AVC_exception(MFX_ERR_UNDEFINED_BEHAVIOR);

            mfxU8 tmp;
            for (mfxU32 i = 0; i < currentSEI.NumBit / 8; i++)
            {
                hevcGetNBits(m_pbs, m_bitOffset, 8, tmp);
            }
        }
    }
}

void HEVCHeadersBitstream::ParseSEI(mfxPayload *spl)
{
    if (nullptr == spl && BytesLeft() < 2)
       return;

    mfxU32 code;
    mfxI32 payloadType = 0;

    hevcNextBits(m_pbs, m_bitOffset, 8, code);
    while (code  ==  0xFF)
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        hevcGetNBits(m_pbs, m_bitOffset, 8, code);
        payloadType += 255;
        hevcNextBits(m_pbs, m_bitOffset, 8, code);
    }
    mfxI32 last_payload_type_byte;
    hevcGetNBits(m_pbs, m_bitOffset, 8, last_payload_type_byte);
    payloadType += last_payload_type_byte;

    mfxU32 payloadSize = 0;
    hevcNextBits(m_pbs, m_bitOffset, 8, code);
    while( code  ==  0xFF )
    {
        /* fixed-pattern bit string using 8 bits written equal to 0xFF */
        hevcGetNBits(m_pbs, m_bitOffset, 8, code);
        payloadSize += 255;
        hevcNextBits(m_pbs, m_bitOffset, 8, code);
    }
    mfxI32 last_payload_size_byte;
    hevcGetNBits(m_pbs, m_bitOffset, 8, last_payload_size_byte);
    payloadSize += last_payload_size_byte;

    spl->NumBit = payloadSize * 8;
    spl->Type = payloadType;
}

}; // namespace HEVCParser

#endif // #ifdef HDR_SEI_PAYLOAD


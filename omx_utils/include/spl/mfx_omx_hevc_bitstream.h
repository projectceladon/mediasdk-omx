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

#ifndef __MFX_OMX_HEVC_BITSTREAM_H_
#define __MFX_OMX_HEVC_BITSTREAM_H_

#include "mfx_omx_avc_bitstream.h"

namespace HEVCParser
{

class HEVCBaseBitstream : public AVCParser::AVCBaseBitstream
{
public:
    HEVCBaseBitstream() : AVCParser::AVCBaseBitstream() {}
    HEVCBaseBitstream(mfxU8 * const pb, const mfxU32 maxsize) : AVCParser::AVCBaseBitstream(pb, maxsize) {}
    virtual ~HEVCBaseBitstream() {}
};

class HEVCHeadersBitstream : public HEVCBaseBitstream
{
public:

    HEVCHeadersBitstream() : HEVCBaseBitstream() {}
    HEVCHeadersBitstream(mfxU8 * const pb, const mfxU32 maxsize) : HEVCBaseBitstream(pb, maxsize) {}

    void GetSEI(mfxPayload *spl, mfxU32 type);

protected:
    void ParseSEI(mfxPayload *spl);
};

} // namespace HEVCParser

#endif // __MFX_OMX_HEVC_BITSTREAM_H_

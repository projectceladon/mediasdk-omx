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

#ifndef __MFX_OMX_COLOR_ASPECTS_WRAPPER_H__
#define __MFX_OMX_COLOR_ASPECTS_WRAPPER_H__

#include "mfx_omx_utils.h"

class MfxOmxColorAspectsWrapper
{
public:

    MfxOmxColorAspectsWrapper();
    ~MfxOmxColorAspectsWrapper();

    void SetFrameworkColorAspects(const android::ColorAspects &colorAspects);
    void UpdateBitsreamColorAspects(const mfxExtVideoSignalInfo &signalInfo);
    void GetOutputColorAspects(android::ColorAspects &outColorAspects);

    bool IsColorAspectsCnahged();
    void SignalChangedColorAspectsIsSent();

private:

    void MfxToOmxVideoRange(mfxU16 videoRange);
    void MfxToOmxColourPrimaries(mfxU16 colourPrimaries);
    void MfxToOmxTransferCharacteristics(mfxU16 transferCharacteristics);
    void MfxToOmxMatrixCoefficients(mfxU16 MatrixCoefficients);

private:

    // Color aspects passed from the framework.
    android::ColorAspects m_frameworkColorAspects;
    // Color aspects parsed from the bitstream.
    android::ColorAspects m_bitstreamColorAspects;

    bool m_bIsColorAspectsChanged;

    MFX_OMX_CLASS_NO_COPY(MfxOmxColorAspectsWrapper)
};

#endif // __MFX_OMX_COLOR_ASPECTS_WRAPPER_H__

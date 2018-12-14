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

#include "mfx_omx_color_aspects_wrapper.h"

MfxOmxColorAspectsWrapper::MfxOmxColorAspectsWrapper()
    : m_bIsColorAspectsChanged(false)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    // Init all the color aspects to be Unspecified.
    memset(&m_frameworkColorAspects, 0, sizeof(android::ColorAspects));
    memset(&m_bitstreamColorAspects, 0, sizeof(android::ColorAspects));
}

MfxOmxColorAspectsWrapper::~MfxOmxColorAspectsWrapper()
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

void MfxOmxColorAspectsWrapper::SetFrameworkColorAspects(const android::ColorAspects &colorAspects)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_frameworkColorAspects = colorAspects;

    MFX_OMX_AUTO_TRACE_I32(m_frameworkColorAspects.mRange);
    MFX_OMX_AUTO_TRACE_I32(m_frameworkColorAspects.mPrimaries);
    MFX_OMX_AUTO_TRACE_I32(m_frameworkColorAspects.mTransfer);
    MFX_OMX_AUTO_TRACE_I32(m_frameworkColorAspects.mMatrixCoeffs);
}

void MfxOmxColorAspectsWrapper::UpdateBitsreamColorAspects(const mfxExtVideoSignalInfo &signalInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_I32(signalInfo.VideoFullRange);
    MFX_OMX_AUTO_TRACE_I32(signalInfo.ColourPrimaries);
    MFX_OMX_AUTO_TRACE_I32(signalInfo.TransferCharacteristics);
    MFX_OMX_AUTO_TRACE_I32(signalInfo.MatrixCoefficients);

    MfxToOmxVideoRange(signalInfo.VideoFullRange);
    MfxToOmxColourPrimaries(signalInfo.ColourPrimaries);
    MfxToOmxTransferCharacteristics(signalInfo.TransferCharacteristics);
    MfxToOmxMatrixCoefficients(signalInfo.MatrixCoefficients);

    if ((m_bitstreamColorAspects.mRange        != android::ColorAspects::RangeUnspecified &&
         m_bitstreamColorAspects.mRange        != m_frameworkColorAspects.mRange)     ||
        (m_bitstreamColorAspects.mPrimaries    != android::ColorAspects::PrimariesUnspecified &&
         m_bitstreamColorAspects.mPrimaries    != m_frameworkColorAspects.mPrimaries) ||
        (m_bitstreamColorAspects.mTransfer     != android::ColorAspects::TransferUnspecified &&
         m_bitstreamColorAspects.mTransfer     != m_frameworkColorAspects.mTransfer)  ||
        (m_bitstreamColorAspects.mMatrixCoeffs != android::ColorAspects::MatrixUnspecified &&
         m_bitstreamColorAspects.mMatrixCoeffs != m_frameworkColorAspects.mMatrixCoeffs)
        )
    {
        m_bIsColorAspectsChanged = true;
    }

    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mRange);
    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mPrimaries);
    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mTransfer);
    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mMatrixCoeffs);
}

void MfxOmxColorAspectsWrapper::GetOutputColorAspects(android::ColorAspects &outColorAspects)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    // The component SHALL return the final color aspects
    // by replacing Unspecified coded values with the default values
    // (default values == sent from framework by OMX_SetConfig)

    if (m_bitstreamColorAspects.mRange != android::ColorAspects::RangeUnspecified)
        outColorAspects.mRange = m_bitstreamColorAspects.mRange;
    else
        outColorAspects.mRange = m_frameworkColorAspects.mRange;

    if (m_bitstreamColorAspects.mPrimaries != android::ColorAspects::PrimariesUnspecified)
        outColorAspects.mPrimaries = m_bitstreamColorAspects.mPrimaries;
    else
        outColorAspects.mPrimaries = m_frameworkColorAspects.mPrimaries;

    if (m_bitstreamColorAspects.mTransfer != android::ColorAspects::TransferUnspecified)
        outColorAspects.mTransfer = m_bitstreamColorAspects.mTransfer;
    else
        outColorAspects.mTransfer = m_frameworkColorAspects.mTransfer;

    if (m_bitstreamColorAspects.mMatrixCoeffs != android::ColorAspects::MatrixUnspecified)
        outColorAspects.mMatrixCoeffs = m_bitstreamColorAspects.mMatrixCoeffs;
    else
        outColorAspects.mMatrixCoeffs = m_frameworkColorAspects.mMatrixCoeffs;

    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mRange);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mPrimaries);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mTransfer);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mMatrixCoeffs);
}

bool MfxOmxColorAspectsWrapper::IsColorAspectsCnahged()
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(m_bIsColorAspectsChanged);

    return m_bIsColorAspectsChanged;
}

void MfxOmxColorAspectsWrapper::SignalChangedColorAspectsIsSent()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_bIsColorAspectsChanged = false;
}


// Private methods - converters VideoSignalInfo from MFX to OMX API

void MfxOmxColorAspectsWrapper::MfxToOmxVideoRange(mfxU16 videoRange)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(videoRange);

    switch (videoRange)
    {
        case 0:
            m_bitstreamColorAspects.mRange = android::ColorAspects::RangeLimited;
            break;

        case 1:
            m_bitstreamColorAspects.mRange = android::ColorAspects::RangeFull;
            break;

        default:
            m_bitstreamColorAspects.mRange = android::ColorAspects::RangeUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mRange);
}

void MfxOmxColorAspectsWrapper::MfxToOmxColourPrimaries(mfxU16 colourPrimaries)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(colourPrimaries);

    switch (colourPrimaries)
    {
        case 1:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesBT709_5;
            break;

        case 2:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesUnspecified;
            break;

        case 4:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesBT470_6M;
            break;

        case 5:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesBT601_6_625;
            break;

        case 6:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesBT601_6_525;
            break;

        case 8:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesGenericFilm;
            break;

        case 9:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesBT2020;
            break;

        default:
            m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mPrimaries);
}

void MfxOmxColorAspectsWrapper::MfxToOmxTransferCharacteristics(mfxU16 transferCharacteristics)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(transferCharacteristics);

    switch (transferCharacteristics)
    {
        case 1:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferSMPTE170M;
            break;

        case 2:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferUnspecified;
            break;

        case 4:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferGamma22;
            break;

        case 5:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferGamma28;
            break;

        case 6:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferSMPTE170M;
            break;

        case 7:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferSMPTE240M;
            break;

        case 8:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferLinear;
            break;

        case 11:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferXvYCC;
            break;

        case 12:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferBT1361;
            break;

        case 13:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferSRGB;
            break;

        case 14:
        case 15:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferSMPTE170M;
            break;

        case 16:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferST2084;
            break;

        case 17:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferST428;
            break;

        case 18:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferHLG;
            break;

        default:
            m_bitstreamColorAspects.mTransfer = android::ColorAspects::TransferUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mTransfer);
}

void MfxOmxColorAspectsWrapper::MfxToOmxMatrixCoefficients(mfxU16 matrixCoefficients)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(matrixCoefficients);

    switch (matrixCoefficients)
    {
        case 1:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixBT709_5;
            break;

        case 2:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixUnspecified;
            break;

        case 4:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixBT470_6M;
            break;

        case 5:
        case 6:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixBT601_6;
            break;

        case 7:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixSMPTE240M;
            break;

        case 9:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixBT2020;
            break;

        case 10:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixBT2020Constant;
            break;

        default:
            m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(m_bitstreamColorAspects.mMatrixCoeffs);
}

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

    m_codecId = MFX_CODEC_AVC;

    // Init all the color aspects to be Unspecified.
    memset(&m_frameworkColorAspects, 0, sizeof(android::ColorAspects));
    memset(&m_bitstreamColorAspects, 0, sizeof(android::ColorAspects));
}

MfxOmxColorAspectsWrapper::~MfxOmxColorAspectsWrapper()
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

void MfxOmxColorAspectsWrapper::SetCodecID(mfxU32 codecId)
{
    MFX_OMX_AUTO_TRACE_FUNC();

	m_codecId = codecId;
	MFX_OMX_AUTO_TRACE_I32(m_codecId);
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

    MfxToOmxVideoRange(signalInfo.VideoFullRange, m_bitstreamColorAspects.mRange);
    MfxToOmxColourPrimaries(signalInfo.ColourPrimaries, m_bitstreamColorAspects.mPrimaries);
    MfxToOmxTransferCharacteristics(signalInfo.TransferCharacteristics, m_bitstreamColorAspects.mTransfer);
    MfxToOmxMatrixCoefficients(signalInfo.MatrixCoefficients, m_bitstreamColorAspects.mMatrixCoeffs);

    mfxU16 video_signal_type_present_flag = signalInfo.VideoFormat != 5 ||
		                                    signalInfo.VideoFullRange != 0 ||
					                        signalInfo.ColourDescriptionPresent != 0;

	if (MFX_CODEC_VP9 == m_codecId || MFX_CODEC_VP8 == m_codecId)
	{
	    video_signal_type_present_flag = false;
	}

	if (!video_signal_type_present_flag)
	{
	    m_bitstreamColorAspects.mRange = android::ColorAspects::RangeUnspecified;
        m_bitstreamColorAspects.mPrimaries = android::ColorAspects::PrimariesUnspecified;
        m_bitstreamColorAspects.mTransfer  = android::ColorAspects::TransferUnspecified;
        m_bitstreamColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixUnspecified;
	}

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

void MfxOmxColorAspectsWrapper::GetColorAspectsFromVideoSignal(const mfxExtVideoSignalInfo &signalInfo, android::ColorAspects &outColorAspects)
{
    bool video_signal_type_present_flag = signalInfo.VideoFormat != 5 ||
                                            signalInfo.VideoFullRange != 0 ||
					                        signalInfo.ColourDescriptionPresent != 0;

	if (MFX_CODEC_VP9 == m_codecId || MFX_CODEC_VP8 == m_codecId)
	{
	    // No video signal info present in vpx bitstream.
	    video_signal_type_present_flag = false;
	}

	if (video_signal_type_present_flag)
	{
		MfxToOmxColourPrimaries(signalInfo.ColourPrimaries, outColorAspects.mPrimaries);
        MfxToOmxVideoRange(signalInfo.VideoFullRange, outColorAspects.mRange);
        MfxToOmxTransferCharacteristics(signalInfo.TransferCharacteristics, outColorAspects.mTransfer);
        MfxToOmxMatrixCoefficients(signalInfo.MatrixCoefficients, outColorAspects.mMatrixCoeffs);
	}
	else
	{
        outColorAspects.mPrimaries = android::ColorAspects::PrimariesUnspecified;
        outColorAspects.mRange = android::ColorAspects::RangeUnspecified;
        outColorAspects.mTransfer = android::ColorAspects::TransferUnspecified;
        outColorAspects.mMatrixCoeffs = android::ColorAspects::MatrixUnspecified;
    }

    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mRange);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mPrimaries);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mTransfer);
    MFX_OMX_AUTO_TRACE_I32(outColorAspects.mMatrixCoeffs);
}

void MfxOmxColorAspectsWrapper::ConvertFrameworkColorAspectToCodecColorAspect(const android::ColorAspects &colorAspects, OMX_VIDEO_PARAM_COLOR_ASPECT &colorParams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    OmxToMfxColourPrimaries(colorAspects.mPrimaries, colorParams.nColourPrimaries);
	OmxToMfxVideoRange(colorAspects.mRange, colorParams.nVideoFullRange);
	OmxToMfxTransferCharacteristics(colorAspects.mTransfer, colorParams.nTransferCharacteristics);
	OmxToMfxMatrixCoefficients(colorAspects.mMatrixCoeffs, colorParams.nMatrixCoefficients);

    MFX_OMX_AUTO_TRACE_I32(colorParams.nColourPrimaries);
    MFX_OMX_AUTO_TRACE_I32(colorParams.nVideoFullRange);
    MFX_OMX_AUTO_TRACE_I32(colorParams.nTransferCharacteristics);
    MFX_OMX_AUTO_TRACE_I32(colorParams.nMatrixCoefficients);
}

bool MfxOmxColorAspectsWrapper::IsColorAspectsChanged()
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

void MfxOmxColorAspectsWrapper::MfxToOmxVideoRange(mfxU16 videoRange, android::ColorAspects::Range &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(videoRange);

    switch (videoRange)
    {
        case 0:
            out = android::ColorAspects::RangeLimited;
            break;

        case 1:
            out = android::ColorAspects::RangeFull;
            break;

        default:
            out = android::ColorAspects::RangeUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(out);
}

void MfxOmxColorAspectsWrapper::MfxToOmxColourPrimaries(mfxU16 colourPrimaries, android::ColorAspects::Primaries &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(colourPrimaries);

    switch (colourPrimaries)
    {
        case 1:
            out = android::ColorAspects::PrimariesBT709_5;
            break;

        case 2:
            out = android::ColorAspects::PrimariesUnspecified;
            break;

        case 4:
            out = android::ColorAspects::PrimariesBT470_6M;
            break;

        case 5:
            out = android::ColorAspects::PrimariesBT601_6_625;
            break;

        case 6:
            out = android::ColorAspects::PrimariesBT601_6_525;
            break;

        case 8:
            out = android::ColorAspects::PrimariesGenericFilm;
            break;

        case 9:
            out = android::ColorAspects::PrimariesBT2020;
            break;

        default:
            out = android::ColorAspects::PrimariesUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(out);
}

void MfxOmxColorAspectsWrapper::MfxToOmxTransferCharacteristics(mfxU16 transferCharacteristics, android::ColorAspects::Transfer &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(transferCharacteristics);

    switch (transferCharacteristics)
    {
        case 1:
            out = android::ColorAspects::TransferSMPTE170M;
            break;

        case 2:
            out = android::ColorAspects::TransferUnspecified;
            break;

        case 4:
            out = android::ColorAspects::TransferGamma22;
            break;

        case 5:
            out = android::ColorAspects::TransferGamma28;
            break;

        case 6:
            out = android::ColorAspects::TransferSMPTE170M;
            break;

        case 7:
            out = android::ColorAspects::TransferSMPTE240M;
            break;

        case 8:
            out = android::ColorAspects::TransferLinear;
            break;

        case 11:
            out = android::ColorAspects::TransferXvYCC;
            break;

        case 12:
            out = android::ColorAspects::TransferBT1361;
            break;

        case 13:
            out = android::ColorAspects::TransferSRGB;
            break;

        case 14:
        case 15:
            out = android::ColorAspects::TransferSMPTE170M;
            break;

        case 16:
            out = android::ColorAspects::TransferST2084;
            break;

        case 17:
            out = android::ColorAspects::TransferST428;
            break;

        case 18:
            out = android::ColorAspects::TransferHLG;
            break;

        default:
            out = android::ColorAspects::TransferUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(out);
}

void MfxOmxColorAspectsWrapper::MfxToOmxMatrixCoefficients(mfxU16 matrixCoefficients, android::ColorAspects::MatrixCoeffs &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(matrixCoefficients);

    switch (matrixCoefficients)
    {
        case 1:
            out = android::ColorAspects::MatrixBT709_5;
            break;

        case 2:
            out = android::ColorAspects::MatrixUnspecified;
            break;

        case 4:
            out = android::ColorAspects::MatrixBT470_6M;
            break;

        case 5:
        case 6:
            out = android::ColorAspects::MatrixBT601_6;
            break;

        case 7:
            out = android::ColorAspects::MatrixSMPTE240M;
            break;

        case 9:
            out = android::ColorAspects::MatrixBT2020;
            break;

        case 10:
            out = android::ColorAspects::MatrixBT2020Constant;
            break;

        default:
            out = android::ColorAspects::MatrixUnspecified;
            break;
    }

    MFX_OMX_AUTO_TRACE_I32(out);
}

void MfxOmxColorAspectsWrapper::OmxToMfxVideoRange(android::ColorAspects::Range videoRange, mfxU16 &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    switch (videoRange)
    {
        case android::ColorAspects::RangeLimited:
			out = 0;
			break;

        case android::ColorAspects::RangeFull:
            out = 1;
            break;

        case android::ColorAspects::RangeUnspecified:
            out = 0xFF;
            break;

        default:
            // should never happen there
            out = 0;
            ALOGE("Unsupported video range");
            break;
    }
}

void MfxOmxColorAspectsWrapper::OmxToMfxColourPrimaries(android::ColorAspects::Primaries colourPrimaries, mfxU16 &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    switch (colourPrimaries)
    {
        case android::ColorAspects::PrimariesBT709_5:
            out = 1;
            break;

        case android::ColorAspects::PrimariesUnspecified:
            out = 2;
            break;

        case android::ColorAspects::PrimariesBT470_6M:
            out = 4;
            break;

        case android::ColorAspects::PrimariesBT601_6_625:
            out = 5;
            break;

        case android::ColorAspects::PrimariesBT601_6_525:
            out = 6;
            break;

        case android::ColorAspects::PrimariesGenericFilm:
            out = 8;
            break;

        case android::ColorAspects::PrimariesBT2020:
            out = 9;
            break;

        default:
            out = 0;
            ALOGE("Unsupported colour primaries");
            break;
    }
}

void MfxOmxColorAspectsWrapper::OmxToMfxTransferCharacteristics(android::ColorAspects::Transfer transferCharacteristics, mfxU16 &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    switch (transferCharacteristics)
    {
        case android::ColorAspects::TransferSMPTE170M:
            out = 1;
            break;

        case android::ColorAspects::TransferUnspecified:
            out = 2;
            break;

        case android::ColorAspects::TransferGamma22:
            out = 4;
            break;

        case android::ColorAspects::TransferGamma28:
            out = 5;
            break;

        case android::ColorAspects::TransferSMPTE240M:
            out = 7;
            break;

        case android::ColorAspects::TransferLinear:
            out = 8;
            break;

        case android::ColorAspects::TransferXvYCC:
            out = 11;
            break;

        case android::ColorAspects::TransferBT1361:
            out = 12;
            break;

        case android::ColorAspects::TransferSRGB:
            out = 13;
            break;

        case android::ColorAspects::TransferST2084:
            out = 16;
            break;

        case android::ColorAspects::TransferST428:
            out = 17;
            break;

        case android::ColorAspects::TransferHLG:
            out = 18;
            break;

        default:
            // should never happen there
            out = 0;
            ALOGE("Unsupported transfer characteristic");
            break;
    }
}

void MfxOmxColorAspectsWrapper::OmxToMfxMatrixCoefficients(android::ColorAspects::MatrixCoeffs matrixCoefficients, mfxU16 &out)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    switch (matrixCoefficients)
    {
        case android::ColorAspects::MatrixBT709_5:
            out = 1;
            break;

        case android::ColorAspects::MatrixUnspecified:
            out = 2;
            break;

        case android::ColorAspects::MatrixBT470_6M:
            out = 4;
            break;

        case android::ColorAspects::MatrixBT601_6:
            out = 5;
            break;

        case android::ColorAspects::MatrixSMPTE240M:
            out = 7;
            break;

        case android::ColorAspects::MatrixBT2020:
            out = 9;
            break;

        case android::ColorAspects::MatrixBT2020Constant:
            out = 10;
            break;

        default:
            // should never happen there
            out = 0;
            ALOGE("Unsupported matrix coefficients");
            break;
    }
}

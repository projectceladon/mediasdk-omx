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

#include "mfx_omx_ports.h"
#include "mfx_omx_component.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_ports"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

MfxOmxPortData* mfx_omx_create_port(MfxOmxPortRegData* reg_data)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxPortData* port_data = NULL;

    switch (reg_data->m_port_id)
    {
    case MfxOmxPortVideo_raw:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData));
        break;
    case MfxOmxPortVideo_h264vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_h264vd));
        break;
    case MfxOmxPortVideo_h265vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_h265vd));
        break;
    case MfxOmxPortVideo_h264ve:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_h264ve));
        break;
    case MfxOmxPortVideo_mp2vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_mp2vd));
        break;
    case MfxOmxPortVideo_vc1vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_vc1vd));
        break;
    case MfxOmxPortVideo_vp8vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_vp8vd));
        break;
    case MfxOmxPortVideo_vp9vd:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_vp9vd));
        break;
    case MfxOmxPortVideo_h265ve:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_h265ve));
        break;
    case MfxOmxPortVideo_vp9ve:
        port_data = (MfxOmxPortData*)calloc(1, sizeof(MfxOmxPortData_vp9ve));
        break;
    default:
        break;
    }
    if (port_data)
    {
        port_data->m_port_id = reg_data->m_port_id;
    }
    return port_data;
}

/*------------------------------------------------------------------------------*/

bool mfx_omx_is_index_valid(OMX_INDEXTYPE index, MfxOmxPortId port_id)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_U32(index);
    MFX_OMX_AUTO_TRACE_U32(port_id);

    if (OMX_IndexParamPortDefinition == index) return true;
    if (MfxOmxPortVideo & port_id)
    {
        if (OMX_IndexParamVideoPortFormat == index) return true;
        if (MFX_OMX_IndexConfigPriority == index) return true;
        switch (port_id)
        {
        case MfxOmxPortVideo_h264ve:
            if (OMX_IndexParamVideoAvc == index) return true;
            if (OMX_IndexParamIntelAVCVUI == index) return true;
            if (OMX_IndexParamVideoBitrate == index) return true;
            if (OMX_IndexParamVideoIntraRefresh == index) return true;
            if (OMX_IndexConfigAndroidIntraRefresh == index) { return true; }
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            if (OMX_IndexParamVideoProfileLevelCurrent == index) return true;
            if (OMX_IndexParamNalStreamFormat == index) return true;
            if (OMX_IndexConfigVideoAVCIntraPeriod == index) return true;
            if (OMX_IndexConfigVideoFramerate == index) return true;
            if (OMX_IndexConfigVideoIntraVOPRefresh == index) return true;
            if (OMX_IndexConfigVideoBitrate == index) return true;
            if (OMX_IndexConfigIntelBitrate == index) return true;
            if (OMX_IndexConfigIntelSliceNumbers == index) return true;
            if (OMX_IndexExtRequestBlackFramePointer == index) return true;
            if (OMX_IndexExtTemporalLayerCount == index) return true;
            if (MfxOmx_IndexIntelHRDParameter == index) return true;
            if (MfxOmx_IndexIntelMaxPictureSize == index) return true;
            if (MfxOmx_IndexIntelTargetUsage == index) return true;
            if (MfxOmx_IndexIntelEncoderFrameCropping == index) return true;
            if (MfxOmx_IndexIntelEncoderVUIControl == index) return true;
            if (MfxOmx_IndexIntelEncoderDirtyRect == index) return true;
            if (MfxOmx_IndexIntelUserData == index) return true;
            if (MfxOmx_IndexIntelEncoderDummyFrame == index) return true;
            if (MfxOmx_IndexIntelEncoderSliceNumber == index) return true;
            if (MfxOmx_IndexIntelEnableInternalSkip == index) return true;
            if (MfxOmx_IndexIntelBitrateLimitOff == index) return true;
            if (MfxOmx_IndexIntelNumRefFrame == index) return true;
            if (MfxOmx_IndexIntelGopPicSize == index) return true;
            if (MfxOmx_IndexIntelIdrInterval == index) return true;
            if (MfxOmx_IndexIntelGopRefDist == index) return true;
            if (MfxOmx_IndexIntelLowPower == index) return true;
            break;
        case MfxOmxPortVideo_h264vd:
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            if (OMX_IndexExtEnableErrorReport == index) return true;
            if (OMX_IndexExtOutputErrorBuffers == index) return true;
            if (OMX_IndexParamIntelAVCDecodeSettings == index) return true;
            if (OMX_IndexExtDecoderBufferHandle == index) return true;
            if (MfxOmx_IndexIntelEnableSFC == index) return true;
            break;
        case MfxOmxPortVideo_mp2vd:
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            break;
        case MfxOmxPortVideo_h265vd:
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            break;
        case MfxOmxPortVideo_vp8vd:
        case MfxOmxPortVideo_vp9vd:
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            break;
        case MfxOmxPortVideo_h265ve:
            if (OMX_IndexParamVideoBitrate == index) return true;
            if (OMX_IndexParamVideoProfileLevelQuerySupported == index) return true;
            if (OMX_IndexParamVideoHevc == index) return true;
            if (OMX_IndexConfigVideoIntraVOPRefresh == index) return true;
            if (OMX_IndexConfigVideoBitrate == index) return true;
            if (OMX_IndexConfigVideoFramerate == index) return true;
            if (MfxOmx_IndexIntelNumRefFrame == index) return true;
            if (MfxOmx_IndexIntelGopPicSize == index) return true;
            if (MfxOmx_IndexIntelIdrInterval == index) return true;
            if (MfxOmx_IndexIntelGopRefDist == index) return true;
            if (MfxOmx_IndexIntelDisableDeblockingIdc == index) return true;
            break;
        case MfxOmxPortVideo_vp9ve:
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("Unhandled port_id");
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_MSG("Invalid index!");
    return false;
}

/*------------------------------------------------------------------------------*/

void mfx_omx_adjust_port_definition(OMX_PARAM_PORTDEFINITIONTYPE* pPortDef,
                                    mfxFrameInfo* pFrameInfo,
                                    bool isMetadataMode,
                                    bool isANWBuffer)
{
    if (pPortDef)
    {
        mfxU32 nFrameWidth;
        mfxU32 nFrameHeight;
        if (pFrameInfo)
        {
            nFrameWidth = pFrameInfo->Width;
            nFrameHeight = pFrameInfo->Height;
        }
        else
        {
            nFrameWidth = pPortDef->format.video.nFrameWidth;
            nFrameHeight = pPortDef->format.video.nFrameHeight;
        }

        switch (pPortDef->eDomain)
        {
        case OMX_PortDomainVideo:
            if (OMX_VIDEO_CodingUnused == pPortDef->format.video.eCompressionFormat)
            {
                OMX_COLOR_FORMATTYPE type = pPortDef->format.video.eColorFormat;

                pPortDef->format.video.cMIMEType = mfx_omx_color2mime_type(type);

                switch (static_cast<int> (type))
                {
                case OMX_INTEL_COLOR_Format_NV12:
                case OMX_INTEL_COLOR_Format_P10:
                    if (isMetadataMode)
                    {
                        if (isANWBuffer)
                        {
                            pPortDef->nBufferSize = sizeof(android::VideoNativeMetadata);
                        }
                        else
                        {
                            pPortDef->nBufferSize = sizeof(android::VideoNativeHandleMetadata);
                        }
                    }
                    else
                    {
                        pPortDef->nBufferSize = sizeof(buffer_handle_t);
                    }
                    break;

                case OMX_COLOR_FormatYUV420Planar:
                case OMX_COLOR_FormatYUV420SemiPlanar:
                    pPortDef->format.video.nStride = pPortDef->format.video.nFrameWidth;
                    pPortDef->format.video.nSliceHeight = pPortDef->format.video.nFrameHeight;
                    pPortDef->nBufferSize = 3 * nFrameWidth * nFrameHeight / 2;
                    break;

                case OMX_COLOR_FormatAndroidOpaque:
                    pPortDef->nBufferSize = sizeof(android::MetadataBufferType) + sizeof(buffer_handle_t);
                    break;

                case OMX_COLOR_FormatUnused:
                default:
                    // should not be here
                    pPortDef->nBufferSize = 0;
                    break;
                }
            }
            else
            {
                pPortDef->format.video.cMIMEType = mfx_omx_coding2mime_type(pPortDef->format.video.eCompressionFormat);

                mfxU32 bufferSize = (3 * nFrameWidth * nFrameHeight / 2 + 32 + sizeof(OMX_OTHER_EXTRADATATYPE));

                if (OMX_VIDEO_CodingVP8 == pPortDef->format.video.eCompressionFormat ||
                    OMX_VIDEO_CodingVP9 == pPortDef->format.video.eCompressionFormat)
                {
                    const mfxU32 compressionRatio = (pPortDef->format.video.eCompressionFormat == OMX_VIDEO_CodingVP8) ? 2 : 4;
                    pPortDef->nBufferSize = MFX_OMX_MAX(2048 * 2048 * 3 / 2 / compressionRatio, bufferSize);
                }
                else
                {
                    pPortDef->nBufferSize = MFX_OMX_MAX(bufferSize, 1024 * 1024);
                }
            }
            break;
        default:
          break;
        }
    }
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

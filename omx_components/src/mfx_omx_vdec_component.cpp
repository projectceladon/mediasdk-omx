// Copyright (c) 2011-2019 Intel Corporation
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

#include "mfx_omx_utils.h"
#include "mfx_omx_defaults.h"
#include "mfx_omx_vdec_component.h"
#include "mfx_omx_vaapi_allocator.h"
#include <chrono>
/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_vdec_component"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECIN_FILE "/data/mfx/mfx_omx_decin"
#define MFX_OMX_DECIN_FC_FILE "/data/mfx/mfx_omx_decin_fc"
#define MFX_OMX_DECOUT_FILE "/data/mfx/mfx_omx_decout.yuv"

/*------------------------------------------------------------------------------*/

MfxOmxComponent* MfxOmxVdecComponent::Create(
    OMX_HANDLETYPE self,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags,
    OMX_ERRORTYPE &error)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    MfxOmxVdecComponent* pComponent = NULL;
    error = OMX_ErrorNone;

    MFX_OMX_NEW(pComponent, MfxOmxVdecComponent(omx_res, self, reg_data, flags));
    if (OMX_ErrorNone == omx_res)
    {
        if (pComponent) omx_res = pComponent->Init();
        else omx_res = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone != omx_res)
    {
        error = omx_res;
        MFX_OMX_DELETE(pComponent);
    }
    MFX_OMX_AUTO_TRACE_P(pComponent);
    return pComponent;
}

/*------------------------------------------------------------------------------*/

MfxOmxVdecComponent::MfxOmxVdecComponent(OMX_ERRORTYPE &error,
                                         OMX_HANDLETYPE self,
                                         MfxOmxComponentRegData* reg_data,
                                         OMX_U32 flags):
    MfxOmxComponent(error, self, reg_data, flags),
    m_Implementation(MFX_OMX_IMPLEMENTATION),
    m_pDEC(NULL),
    m_extBuffers{},
    m_bLegacyAdaptivePlayback(false),
    m_nMaxFrameWidth(0),
    m_nMaxFrameHeight(0),
    m_bChangeOutputPortSettings(false),
    m_bEosHandlingStarted(false),
    m_bEosHandlingFinished(false),
    m_bUseSystemMemory(true),
    m_bReinit(false),
    m_bFlush(false),
    m_bErrorReportingEnabled(false),
    m_bEnableVP(false),
    m_bAllocateNativeHandle(false),
    m_bEnableNativeBuffersReceived(false),
    m_bInterlaced(false),
    m_InitState(MFX_INIT_DECODE_HEADER),
    m_pDevice(NULL),
    m_pBufferHeaders(NULL),
    m_nBufferHeadersSize(0),
    m_pBitstream(NULL),
    m_pOmxBitstream(NULL),
    m_nSurfacesNum(1),
    m_nSurfacesNumMin(1),
    m_pSurfaces(NULL),
    m_pFreeSyncPoint(NULL),
    m_nLockedSurfacesNum(0),
    m_nCountDecodedFrames(0),
#ifdef HEVC10HDR_SUPPORT
    m_bIsSetHDRSEI(false),
#endif
    m_dbg_decin(NULL),
    m_dbg_decin_fc(NULL),
    m_dbg_decout(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_ZERO_MEMORY(m_MfxVideoParams);
    MFX_OMX_ZERO_MEMORY(m_Crops);
    MFX_OMX_ZERO_MEMORY(m_signalInfo);
    MFX_OMX_ZERO_MEMORY(m_adaptivePlayback);
    MFX_OMX_ZERO_MEMORY(m_decVideoProc);
    MFX_OMX_ZERO_MEMORY(m_AllocResponse);
#ifdef HEVC10HDR_SUPPORT
    MFX_OMX_ZERO_MEMORY(m_SeiHDRStaticInfo);
#endif
}

/*------------------------------------------------------------------------------*/

MfxOmxVdecComponent::~MfxOmxVdecComponent(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    InternalThreadsWait();

    MFX_OMX_FREE(m_pBufferHeaders);
    MFX_OMX_DELETE(m_pOmxBitstream);
    MFX_OMX_DELETE(m_pSurfaces);
    MFX_OMX_DELETE(m_pDEC);
    m_Session.Close();

    MFX_OMX_DELETE(m_pDevice);

    if (m_dbg_decin) fclose(m_dbg_decin);
    if (m_dbg_decin_fc) fclose(m_dbg_decin_fc);
    if (m_dbg_decout) fclose(m_dbg_decout);

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Decoded %d frames", m_nCountDecodedFrames);
    MFX_OMX_LOG_INFO("Destroyed %s", m_pRegData->m_name);
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::Init(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE error = MfxOmxComponent::Init();
    mfxStatus sts = MFX_ERR_NONE;

    if (OMX_ErrorNone == error)
    {
        Reset();

        // prepare Media SDK
        sts = m_Session.Init(m_Implementation, &g_MfxVersion);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Session.Init sts %d, version %d.%d", sts, g_MfxVersion.Major, g_MfxVersion.Minor);

        if (MFX_ERR_NONE == sts)
        {
            sts = m_Session.QueryIMPL(&m_Implementation);
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Session.QueryIMPL sts %d, impl 0x%04x", sts, m_Implementation);
        }

        // decoder creation
        if (MFX_ERR_NONE == sts)
        {
            MFX_OMX_NEW(m_pDEC, MFXVideoDECODE(m_Session));
            if (!m_pDEC) sts = MFX_ERR_MEMORY_ALLOC;
        }
        if (MFX_ERR_NONE != sts) error = ErrorStatusMfxToOmx(sts);
    }
    if ((OMX_ErrorNone == error) && (MFX_IMPL_SOFTWARE != m_Implementation))
    {
        // driver initialization
        m_pDevice = mfx_omx_create_dev();
        if (m_pDevice) sts = m_pDevice->DevInit();
        else sts = MFX_ERR_DEVICE_FAILED;

        if (MFX_ERR_NONE != sts)
        {
            MFX_OMX_DELETE(m_pDevice);
            error = OMX_ErrorHardware;
        }
        MFX_OMX_AUTO_TRACE_P(m_pDevice);
    }
    if ((OMX_ErrorNone == error) && (MFX_IMPL_SOFTWARE != m_Implementation))
    {
        sts = m_pDevice->InitMfxSession(&m_Session);
        if (MFX_ERR_NONE != sts) error = ErrorStatusMfxToOmx(sts);
    }
    if (OMX_ErrorNone == error)
    {
        MfxOmxFrameConstructorType fc_type;
        switch (m_pRegData->m_type)
        {
        case MfxOmx_h264vd:
            fc_type = MfxOmxFC_AVC;
            break;
        case MfxOmx_h265vd:
            fc_type = MfxOmxFC_HEVC;
            break;
        case MfxOmx_vc1vd:
            fc_type = MfxOmxFC_VC1;
            break;
        case MfxOmx_vp8vd:
            fc_type = MfxOmxFC_VP8;
            break;
        case MfxOmx_vp9vd:
            fc_type = MfxOmxFC_VP9;
            break;
        default:
            fc_type = MfxOmxFC_None;
            break;
        }
        if (MFX_ERR_NONE == sts)
        {
            MFX_OMX_NEW(m_pOmxBitstream, MfxOmxBitstream(fc_type, sts));
        }

        if (MFX_ERR_NONE == sts)
        {
            if (m_pOmxBitstream)
            {
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
                if (MFX_OMX_COMPONENT_FLAGS_DUMP_INPUT & m_Flags)
                {
                    m_dbg_decin = fopen(MFX_OMX_DECIN_FILE, "w");
                    m_dbg_decin_fc = fopen(MFX_OMX_DECIN_FC_FILE, "w");
                }
#endif
                m_pOmxBitstream->GetFrameConstructor()->SetFiles(m_dbg_decin, m_dbg_decin_fc);
            }
            else sts = MFX_ERR_NULL_PTR;
        }
        if (MFX_ERR_NONE == sts)
        {
            sts = m_pOmxBitstream->SetBuffersCallback(this);
        }
        if (MFX_ERR_NONE != sts)
        {
            if (MFX_ERR_UNSUPPORTED == sts) error = OMX_ErrorComponentNotFound;
            else error = ErrorStatusMfxToOmx(sts);
        }
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pSurfaces, MfxOmxSurfacesPool(m_MfxVideoParams.mfx.CodecId, sts));
        if ((MFX_ERR_NONE == sts) && !m_pSurfaces)
        {
            sts = MFX_ERR_NULL_PTR;
        }
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
        if (MFX_ERR_NONE == sts)
        {
            if (MFX_OMX_COMPONENT_FLAGS_DUMP_OUTPUT & m_Flags)
            {
                m_dbg_decout = fopen(MFX_OMX_DECOUT_FILE, "w");
            }
            m_pSurfaces->SetFile(m_dbg_decout);
        }
#endif
        if (MFX_ERR_NONE == sts)
        {
            sts = m_pSurfaces->SetBuffersCallback(this);
        }
        if ((MFX_ERR_NONE == sts) && (MFX_IMPL_SOFTWARE != m_Implementation))
        {
            m_pSurfaces->SetMfxDevice(m_pDevice);
        }
        if (MFX_ERR_NONE != sts) error = ErrorStatusMfxToOmx(sts);
    }

    if (OMX_ErrorNone == error)
    {
        MFX_OMX_LOG_INFO("Created %s", m_pRegData->m_name);
    }
    else
    {
        MFX_OMX_LOG_ERROR("Failed to create %s, error 0x%x", m_pRegData->m_name, error);
    }
    MFX_OMX_AUTO_TRACE_I32(sts);

    return error;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_ZERO_MEMORY(m_MfxVideoParams);

    MFX_OMX_ZERO_MEMORY(m_signalInfo);
    m_signalInfo.Header.BufferId = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
    m_signalInfo.Header.BufferSz = sizeof(mfxExtVideoSignalInfo);

    MFX_OMX_ZERO_MEMORY(m_adaptivePlayback);
    m_adaptivePlayback.BufferId = MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK;
    m_adaptivePlayback.BufferSz = sizeof(mfxExtBuffer);

    MFX_OMX_ZERO_MEMORY(m_decVideoProc);
    m_decVideoProc.Header.BufferId = MFX_EXTBUFF_DEC_VIDEO_PROCESSING;
    m_decVideoProc.Header.BufferSz = sizeof(mfxExtDecVideoProcessing);

    MFX_OMX_ZERO_MEMORY(m_AllocResponse);
    switch (m_pRegData->m_type)
    {
    case MfxOmx_h265vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_HEVC;
        break;
    case MfxOmx_h264vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
        break;
    case MfxOmx_mp2vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_MPEG2;
        break;
    case MfxOmx_vc1vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_VC1;
        break;
    case MfxOmx_vp8vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_VP8;
        break;
    case MfxOmx_vp9vd:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_VP9;
        break;
    default:
        MFX_OMX_AUTO_TRACE_MSG("Unhandled codec type: error in plug-in registration");
        break;
    }

    m_colorAspects.SetCodecID(m_MfxVideoParams.mfx.CodecId);

    mfx_omx_set_defaults_mfxVideoParam_dec(&m_MfxVideoParams);

    PortsParams_2_MfxVideoParams();
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::InitPort(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::InitPort(nPortIndex);

    if (OMX_ErrorNone == omx_res)
    {
        MfxOmxVideoPortRegData* video_reg = (MfxOmxVideoPortRegData*)(m_pRegData->m_ports[nPortIndex]);

        if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
        {
            if (MfxOmxPortVideo_h264vd == video_reg->m_port_reg_data.m_port_id)
            {
                MfxOmxPortData_h264vd* h264vd_port = (MfxOmxPortData_h264vd*)(m_pPorts[nPortIndex]);

                SetStructVersion<OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS>(&(h264vd_port->m_intel_avc_decode_settings));
                h264vd_port->m_intel_avc_decode_settings.nPortIndex = video_reg->m_port_reg_data.m_port_index;
                MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS(h264vd_port->m_intel_avc_decode_settings);
            }
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::Set_PortDefinition(
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::Set_PortDefinition(pPortDef);

    if (OMX_ErrorNone == omx_res)
    {
        if (MFX_OMX_INPUT_PORT_INDEX == pPortDef->nPortIndex)
        {
            // synchronizing output port parameters
            omx_res = InPortParams_2_OutPortParams();
            // synchronizing with mfxVideoParam
            if (OMX_ErrorNone == omx_res) omx_res = PortsParams_2_MfxVideoParams();
            // synchronizing with frame constructor (needed for vc1)
            if (OMX_ErrorNone == omx_res)
            {
                mfxStatus sts = MFX_ERR_NONE;
                sts = m_pOmxBitstream->InitFrameConstructor(MFX_PROFILE_UNKNOWN, m_MfxVideoParams.mfx.FrameInfo);
                if (MFX_ERR_NONE != sts) omx_res = OMX_ErrorUndefined;
            }

        }
        else if (MFX_OMX_OUTPUT_PORT_INDEX != pPortDef->nPortIndex)
            omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::InPortParams_2_OutPortParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    m_pOutPortDef->format.video.nFrameWidth = m_pInPortDef->format.video.nFrameWidth;
    m_pOutPortDef->format.video.nFrameHeight = m_pInPortDef->format.video.nFrameHeight;
    m_pOutPortDef->format.video.nSliceHeight = m_pInPortDef->format.video.nSliceHeight;

    if (m_pDevice && !m_bUseSystemMemory)
    {
        m_pOutPortDef->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)((MFX_FOURCC_P010 == m_MfxVideoParams.mfx.FrameInfo.FourCC) ?
                                                    OMX_INTEL_COLOR_Format_P10 :
                                                    OMX_INTEL_COLOR_Format_NV12);
    }

    mfx_omx_adjust_port_definition(m_pOutPortDef, &m_MfxVideoParams.mfx.FrameInfo,
                                   m_bOnFlySurfacesAllocation,
                                   m_bANWBufferInMetaData);

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::PortsParams_2_MfxVideoParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    // data from input port
    m_MfxVideoParams.mfx.FrameInfo.Width = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    m_MfxVideoParams.mfx.FrameInfo.Height = (mfxU16)m_pInPortDef->format.video.nFrameHeight;
    m_MfxVideoParams.mfx.FrameInfo.CropW = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    m_MfxVideoParams.mfx.FrameInfo.CropH = (mfxU16)m_pInPortDef->format.video.nFrameHeight;

    m_Crops.CropX = 0;
    m_Crops.CropY = 0;
    m_Crops.CropW = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    m_Crops.CropH = (mfxU16)m_pInPortDef->format.video.nFrameHeight;

    m_MfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    m_MfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    MFX_OMX_AT__mfxVideoParam_dec(m_MfxVideoParams);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::MfxVideoParams_2_PortsParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    // Using cropW/cropH to workaround the app issue when an app doesn't handle OMX_IndexConfigCommonOutputCrop
    mfxU16 nFrameWidth = m_MfxVideoParams.mfx.FrameInfo.CropW;
    mfxU16 nFrameHeight = m_MfxVideoParams.mfx.FrameInfo.CropH;

    // For system memory we use real Width/Height
    if (m_bUseSystemMemory && !m_bLegacyAdaptivePlayback)
    {
        nFrameWidth = m_MfxVideoParams.mfx.FrameInfo.Width;
        nFrameHeight = m_MfxVideoParams.mfx.FrameInfo.Height;
    }
    else if (m_bEnableVP)
    {
        nFrameWidth = m_nMaxFrameWidth = m_decVideoProc.Out.Width;
        nFrameHeight = m_nMaxFrameHeight = m_decVideoProc.Out.Height;
    }
    else if (m_bLegacyAdaptivePlayback)
    {
        if (m_nMaxFrameWidth >= m_MfxVideoParams.mfx.FrameInfo.Width) nFrameWidth = m_nMaxFrameWidth;
        if (m_nMaxFrameHeight >= m_MfxVideoParams.mfx.FrameInfo.Height) nFrameHeight = m_nMaxFrameHeight;
    }

    m_pOutPortDef->format.video.nFrameWidth = nFrameWidth;
    m_pOutPortDef->format.video.nFrameHeight = nFrameHeight;
    m_pOutPortDef->format.video.nSliceHeight = nFrameHeight;

    if (m_bChangeOutputPortSettings)
    {
        m_pOutPortDef->bPopulated = OMX_FALSE;
        m_pOutPortDef->nBufferCountMin = m_nSurfacesNumMin;
        m_pOutPortDef->nBufferCountActual = m_nSurfacesNum;
    }

    if (m_pDevice && !m_bUseSystemMemory)
    {
        m_pOutPortDef->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)((MFX_FOURCC_P010 == m_MfxVideoParams.mfx.FrameInfo.FourCC) ?
                                                    OMX_INTEL_COLOR_Format_P10 :
                                                    OMX_INTEL_COLOR_Format_NV12);
    }

    MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE((*m_pOutPortDef));

    mfx_omx_adjust_port_definition(m_pOutPortDef, NULL,
                                   m_bOnFlySurfacesAllocation,
                                   m_bANWBufferInMetaData);

    MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE((*m_pOutPortDef));

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

void* MfxOmxVdecComponent::AllocInputBuffer(size_t nSizeBytes)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(nSizeBytes);
    return malloc(nSizeBytes);
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::FreeInputBuffer(void* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (pBuffer)
    {
        free(pBuffer);
        pBuffer = NULL;
    }
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::DealWithBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::DealWithBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    // using buffer
    if (OMX_ErrorNone == omx_res)
    {
        mfxStatus mfx_sts = MFX_ERR_NONE;
        // getting port information
        OMX_PARAM_PORTDEFINITIONTYPE* port_def = NULL;
        MFX_OMX_PARAM_PORTINFOTYPE* port_info = NULL;
        // allocating buffer
        OMX_BUFFERHEADERTYPE* pBufferHeader = (OMX_BUFFERHEADERTYPE*)calloc(1, sizeof(OMX_BUFFERHEADERTYPE));
        MfxOmxBufferInfo* pBufInfo = (MfxOmxBufferInfo*)calloc(1, sizeof(MfxOmxBufferInfo));
        OMX_U8* pData = NULL;

        if (!pBuffer) pData = (OMX_U8*)AllocInputBuffer(nSizeBytes);
        if (pBufferHeader && pBufInfo && (pBuffer || pData))
        {
            SetStructVersion<OMX_BUFFERHEADERTYPE>(pBufferHeader);
            pBufferHeader->nAllocLen = nSizeBytes;
            if (pBuffer)
            {
                pBufferHeader->pBuffer = pBuffer;
                pBufInfo->bSelfAllocatedBuffer = false;
            }
            else
            {
                pBufferHeader->pBuffer = pData;
                pBufInfo->bSelfAllocatedBuffer = true;
            }
            pBufferHeader->pAppPrivate = pAppPrivate;
            if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
            {
                port_def = m_pInPortDef;
                port_info = m_pInPortInfo;

                pBufInfo->eType = MfxOmxBuffer_Bitstream;

                pBufferHeader->nInputPortIndex = nPortIndex;
                pBufferHeader->pInputPortPrivate = pBufInfo;
            }
            else
            {
                port_def = m_pOutPortDef;
                port_info = m_pOutPortInfo;

                pBufInfo->bUsed = true;
                pBufInfo->eType = MfxOmxBuffer_Surface;
                pBufInfo->nBufferIndex = port_info->nCurrentBuffersCount;
                MFX_OMX_AUTO_TRACE_U32(pBufInfo->nBufferIndex);

                pBufferHeader->nOutputPortIndex = nPortIndex;
                pBufferHeader->pOutputPortPrivate = pBufInfo;

                if (!m_pBufferHeaders)
                {
                    m_pBufferHeaders = (OMX_BUFFERHEADERTYPE**)calloc(port_def->nBufferCountActual, sizeof(OMX_BUFFERHEADERTYPE*));
                    m_nBufferHeadersSize = port_def->nBufferCountActual;
                    MFX_OMX_AUTO_TRACE_P(m_pBufferHeaders);
                    if (!m_pBufferHeaders) mfx_sts = MFX_ERR_MEMORY_ALLOC;
                }
                if (MFX_ERR_NONE == mfx_sts)
                {
                    mfxU32 sync_points_num = m_SyncPoints.GetItemsCount();

                    if (m_pFreeSyncPoint) ++sync_points_num;
                    // number of sync points should be equal to number of buffers
                    if (port_info->nCurrentBuffersCount >= sync_points_num)
                    {
                        mfxSyncPoint* pSyncPoint = (mfxSyncPoint*)calloc(1, sizeof(mfxSyncPoint));

                        if (!pSyncPoint || !m_SyncPoints.Add(&pSyncPoint))
                        {
                            MFX_OMX_FREE(pSyncPoint);
                            mfx_sts = MFX_ERR_MEMORY_ALLOC;
                        }
                    }
                    m_pBufferHeaders[port_info->nCurrentBuffersCount] = pBufferHeader;
                }
                m_pSurfaces->SetMaxErrorCount(port_info->nCurrentBuffersCount+1);
            }
        }
        else 
        {
            MFX_OMX_LOG_ERROR("Failed to allocate buffers for decode output, pBufferHeader=(%p), pBuffer=(%p), nTimeStamp %lld", pBufferHeader, pBuffer, (*ppBufferHdr)->nTimeStamp);
            mfx_sts = MFX_ERR_MEMORY_ALLOC;
        }

        if (MFX_ERR_NONE == mfx_sts)
        {
            ++(port_info->nCurrentBuffersCount);
            if (port_info->nCurrentBuffersCount == port_def->nBufferCountActual)
            {
                port_def->bPopulated = OMX_TRUE;
            }
            if (IsStateTransitionReady())
            {
                MFX_OMX_AUTO_TRACE_MSG("Sending m_pStateTransitionEvent");
                m_pStateTransitionEvent->Signal();
            }
            *ppBufferHdr = pBufferHeader;

            MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBufferHeader));
        }
        else
        {
            MFX_OMX_FREE(pBufferHeader);
            MFX_OMX_FREE(pBufInfo);
            FreeInputBuffer(pData);
            omx_res = OMX_ErrorInsufficientResources;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::FreeBuffer(
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::FreeBuffer(nPortIndex, pBuffer);

    // using buffer
    if (OMX_ErrorNone == omx_res)
    {
        OMX_PARAM_PORTDEFINITIONTYPE* port_def = NULL;
        MFX_OMX_PARAM_PORTINFOTYPE* port_info = NULL;
        MfxOmxBufferInfo* buf_info = NULL;

        MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBuffer));

        if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
        {
            buf_info = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;
            port_def = m_pInPortDef;
            port_info = m_pInPortInfo;
        }
        else
        {
            buf_info = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;
            port_def = m_pOutPortDef;
            port_info = m_pOutPortInfo;
        }
        if (buf_info->bSelfAllocatedBuffer)
        {
            FreeInputBuffer(pBuffer->pBuffer);
        }
        if (buf_info->pAnwBuffer)
        {
            buf_info->bUsed = false;
        }
        else
        {
            MFX_OMX_FREE(buf_info);
        }
        MFX_OMX_FREE(pBuffer);

        --(port_info->nCurrentBuffersCount);
        if (port_info->nCurrentBuffersCount != port_def->nBufferCountActual)
        {
            port_def->bPopulated = OMX_FALSE;
        }

        if (IsStateTransitionReady())
        {
            MFX_OMX_AUTO_TRACE_MSG("Sending m_pStateTransitionEvent");
            m_pStateTransitionEvent->Signal();
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::EmptyThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EmptyThisBuffer(%p) nTimeStamp %lld, nFilledLen %d, nFlags 0x%x",
                        pBuffer, pBuffer->nTimeStamp, pBuffer->nFilledLen, pBuffer->nFlags);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::EmptyThisBuffer(pBuffer);

    // emptying buffer
    if (OMX_ErrorNone == omx_res)
    {
        if (MFX_ERR_NONE == m_pOmxBitstream->UseBuffer(pBuffer))
        {
            m_pCommandsSemaphore->Post();
        }
        else omx_res = OMX_ErrorUndefined;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::FillThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "FillThisBuffer(%p) nAllocLen %d", pBuffer, pBuffer->nAllocLen);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::FillThisBuffer(pBuffer);

    if (OMX_ErrorNone == omx_res)
    {
        mfxStatus mfx_res = m_pSurfaces->UseBuffer(pBuffer, m_MfxVideoParams.mfx.FrameInfo, m_bChangeOutputPortSettings);
        if (MFX_ERR_NONE == mfx_res)
        {
            m_pCommandsSemaphore->Post();
        }
        else
        {
            omx_res = ErrorStatusMfxToOmx(mfx_res);
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, omx_res, 0 , NULL);
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::GetParameter(nParamIndex, pComponentParameterStructure);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (static_cast<int>(nParamIndex))
        {
        case OMX_IndexParamPortDefinition:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamPortDefinition");
                OMX_PARAM_PORTDEFINITIONTYPE* pParam = (OMX_PARAM_PORTDEFINITIONTYPE*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_PARAM_PORTDEFINITIONTYPE>(pParam, sizeof(OMX_PARAM_PORTDEFINITIONTYPE), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsPortValid(pParam->nPortIndex))
                    {
                        MfxOmxAutoLock lock(m_decoderMutex);
                        OMX_PARAM_PORTDEFINITIONTYPE & port = m_pPorts[pParam->nPortIndex]->m_port_def;
                        *pParam = port;
                        m_Crops = m_MfxVideoParams.mfx.FrameInfo;

                        MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE(port);
                        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "GetParameter(ParamPortDefinition) nPortIndex %d, nBufferCountMin %d, nBufferCountActual %d, nBufferSize %d, nFrameWidth %d, nFrameHeight %d",
                                            port.nPortIndex, port.nBufferCountMin, port.nBufferCountActual, port.nBufferSize, port.format.video.nFrameWidth, port.format.video.nFrameHeight);
                        omx_res = OMX_ErrorNone;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoPortFormat");
                OMX_VIDEO_PARAM_PORTFORMATTYPE* pParam = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_VIDEO_PARAM_PORTFORMATTYPE>(pParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(OMX_IndexParamVideoPortFormat, pParam->nPortIndex))
                    {
                        MfxOmxVideoPortRegData* video_reg = (MfxOmxVideoPortRegData*)(m_pRegData->m_ports[pParam->nPortIndex]);
                        OMX_U32 format_index = pParam->nIndex;
                        MFX_OMX_AUTO_TRACE_I32(format_index);

                        OMX_U32 formats_num = 0;
                        OMX_VIDEO_PARAM_PORTFORMATTYPE* pFormats = NULL;
                        OMX_VIDEO_PARAM_PORTFORMATTYPE formats[video_reg->m_formats_num];
                        if (MFX_OMX_OUTPUT_PORT_INDEX == pParam->nPortIndex && m_bEnableNativeBuffersReceived)
                        {
                            for (mfxU32 i = 0; i < video_reg->m_formats_num; i++)
                            {
                                if (video_reg->m_formats[i].eColorFormat == (m_bUseSystemMemory ? OMX_COLOR_FormatYUV420SemiPlanar : (OMX_COLOR_FORMATTYPE)OMX_INTEL_COLOR_Format_NV12))
                                {
                                    formats[formats_num] = video_reg->m_formats[i];
                                    formats_num++;
                                }
                            }
                            pFormats = formats;
                        }
                        else
                        {
                            formats_num = video_reg->m_formats_num;
                            pFormats = video_reg->m_formats;
                        }

                        if (format_index < formats_num)
                        {
                            *pParam = pFormats[format_index];
                            pParam->nIndex = format_index; // returning this value back

                            MFX_OMX_AT__OMX_VIDEO_PARAM_PORTFORMATTYPE(pFormats[format_index]);

                            omx_res = OMX_ErrorNone;
                        }
                        else omx_res = OMX_ErrorNoMore;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGoogleGetNativeBufferUsage:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: Omx_IndexGoogleGetNativeBufferUsage");
                android::GetAndroidNativeBufferUsageParams* pParam = (android::GetAndroidNativeBufferUsageParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::GetAndroidNativeBufferUsageParams>(pParam, sizeof(android::GetAndroidNativeBufferUsageParams), OMX_VERSION))
                {
                    pParam->nUsage |= GRALLOC_USAGE_HW_TEXTURE;
#if (MFX_ANDROID_VERSION >= MFX_P)
                    if (m_bInterlaced)
                    {
                        pParam->nUsage |= GRALLOC_USAGE_PRIVATE_0;// set interlace mode for HWC
                    }
#endif
                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGooglePrepareForAdaptivePlayback:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrepareForAdaptivePlayback");
                android::PrepareForAdaptivePlaybackParams* pParam = (android::PrepareForAdaptivePlaybackParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::PrepareForAdaptivePlaybackParams>(pParam, sizeof(android::PrepareForAdaptivePlaybackParams), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrepareForAdaptivePlayback is valid");
                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexExtEnableErrorReport:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtEnableErrorReport");
                OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT* pParam = (OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT*)pComponentParameterStructure;
                if (IsStructVersionValid<OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT>(pParam, sizeof(OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexExtEnableErrorReport), pParam->nPortIndex))
                        omx_res = OMX_ErrorNone;
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexExtOutputErrorBuffers:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtOutputErrorBuffers");
                OMX_VIDEO_OUTPUT_ERROR_BUFFERS* pParam = (OMX_VIDEO_OUTPUT_ERROR_BUFFERS*)pComponentParameterStructure;
                if (IsStructVersionValid<OMX_VIDEO_OUTPUT_ERROR_BUFFERS>(pParam, sizeof(OMX_VIDEO_OUTPUT_ERROR_BUFFERS), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexExtOutputErrorBuffers), pParam->nPortIndex))
                    {
                        m_pSurfaces->FillOutputErrorBuffer( &(pParam->errorBuffers), pParam->nErrorBufIndex);
                        omx_res = OMX_ErrorNone;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamIntelAVCDecodeSettings:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamIntelAVCDecodeSettings");
                OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS* pParam = (OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS>(pParam, sizeof(OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexParamIntelAVCDecodeSettings), pParam->nPortIndex))
                    {
                        MfxOmxVideoPortData* video_port = (MfxOmxVideoPortData*)(m_pPorts[pParam->nPortIndex]);
                        *pParam = video_port->m_intel_avc_decode_settings;
                        MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS(video_port->m_intel_avc_decode_settings);
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexIntelDecodedOrder:
            {
                MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelDecodedOrder");
                OMX_CONFIG_DECODEDORDER *pParam = static_cast<OMX_CONFIG_DECODEDORDER *>(pComponentParameterStructure);

                if (IsStructVersionValid<OMX_CONFIG_DECODEDORDER>(pParam, sizeof(OMX_CONFIG_DECODEDORDER), OMX_VERSION))
                {
                    pParam->bDecodedOrder = (1 == m_MfxVideoParams.mfx.DecodedOrder) ? OMX_TRUE : OMX_FALSE;
                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("unknown nParamIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }

    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::SetParameter(
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::SetParameter(nParamIndex, pComponentParameterStructure);

    bool bIncorrectState = (OMX_StateLoaded != m_state) && (OMX_StateWaitForResources != m_state);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (static_cast<int>(nParamIndex))
        {
        case MfxOmx_IndexGoogleStoreANWBufferInMetaData:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGoogleStoreANWBufferInMetaData");
                android::StoreMetaDataInBuffersParams* pParam = (android::StoreMetaDataInBuffersParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::StoreMetaDataInBuffersParams>(pParam, sizeof(android::StoreMetaDataInBuffersParams), OMX_VERSION))
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParameter(MfxOmx_IndexGoogleStoreANWBufferInMetaData) port %d, bStoreMetaData %d", pParam->nPortIndex, pParam->bStoreMetaData);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (MFX_OMX_OUTPUT_PORT_INDEX == pParam->nPortIndex)
                    {
                        MFX_OMX_AUTO_TRACE_I32(pParam->bStoreMetaData);
                        m_bOnFlySurfacesAllocation = pParam->bStoreMetaData;
                        m_bANWBufferInMetaData = true;
                        m_pSurfaces->SetSurfacesAllocationMode(m_bOnFlySurfacesAllocation, m_bANWBufferInMetaData);

                        if (pParam->bStoreMetaData)
                        {
                            mfxFrameAllocator* pAllocator = m_pDevice->GetFrameAllocator();
                            if (pAllocator)
                            {
                                m_pSurfaces->SetFrameAllocator(pAllocator);
                            }
                        }
                    }
                    else
                    {
                        MFX_OMX_AUTO_TRACE_MSG("Input port bStoreMetaData parameter value is skipped by OMX plug-in now");
                    }
                    omx_res = OMX_ErrorNone;
            }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGoogleEnableNativeBuffers :
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: Omx_IndexGoogleEnableNativeBuffers");
                android::EnableAndroidNativeBuffersParams* pParam = (android::EnableAndroidNativeBuffersParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::EnableAndroidNativeBuffersParams>(pParam, sizeof(android::EnableAndroidNativeBuffersParams), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    MFX_OMX_AUTO_TRACE_I32(pParam->enable);
                    if (MFX_OMX_OUTPUT_PORT_INDEX == pParam->nPortIndex)
                    {
                        // Set memory type of output surfaces
                        m_bUseSystemMemory = !pParam->enable;
                        m_bEnableNativeBuffersReceived = true;

                        if (m_bUseSystemMemory)
                        {
                            m_pOutPortDef->format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
                        }
                        else
                        {
                            m_pOutPortDef->format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)((MFX_FOURCC_P010 == m_MfxVideoParams.mfx.FrameInfo.FourCC) ?
                                                                        OMX_INTEL_COLOR_Format_P10 :
                                                                        OMX_INTEL_COLOR_Format_NV12);
                        }
                        MFX_OMX_AUTO_TRACE_I32(m_bUseSystemMemory);

                        mfx_omx_adjust_port_definition(m_pOutPortDef, &m_MfxVideoParams.mfx.FrameInfo,
                                                       m_bOnFlySurfacesAllocation,
                                                       m_bANWBufferInMetaData);
                    }
                    else
                    {
                        MFX_OMX_AUTO_TRACE_MSG("EnableAndroidNativeBuffersParams parameter for input port is skipped by OMX plug-in now");
                    }

                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGoogleAllocateNativeHandle:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: Omx_IndexGoogleAllocateNativeHandle");
                android::EnableAndroidNativeBuffersParams* pParam = (android::EnableAndroidNativeBuffersParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::EnableAndroidNativeBuffersParams>(pParam, sizeof(android::EnableAndroidNativeBuffersParams), OMX_VERSION))
                {
                    m_bAllocateNativeHandle = pParam->enable;
                    MFX_OMX_AUTO_TRACE_I32(m_bAllocateNativeHandle);

                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGooglePrepareForAdaptivePlayback:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrepareForAdaptivePlayback");
                android::PrepareForAdaptivePlaybackParams* pParam = (android::PrepareForAdaptivePlaybackParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::PrepareForAdaptivePlaybackParams>(pParam, sizeof(android::PrepareForAdaptivePlaybackParams), OMX_VERSION))
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParameter(GooglePrepareForAdaptivePlayback) bEnable %d, WxH %dx%d",
                                        pParam->bEnable, pParam->nMaxFrameWidth, pParam->nMaxFrameHeight);
                    MFX_OMX_AUTO_TRACE_I32(pParam->bEnable);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nMaxFrameWidth);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nMaxFrameHeight);

                    if (pParam->bEnable)
                    {
                        m_bLegacyAdaptivePlayback = true;
                        if (MFX_CODEC_VP8 != m_MfxVideoParams.mfx.CodecId)
                        {
                            m_nMaxFrameWidth = MFX_OMX_MEM_ALIGN(pParam->nMaxFrameWidth, 16);
                            m_nMaxFrameHeight = MFX_OMX_MEM_ALIGN(pParam->nMaxFrameHeight, 32);
                        }
                    }
                    else
                        m_bLegacyAdaptivePlayback = false;

                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGoogleUseNativeBuffers:
            {
                omx_res = OMX_ErrorNone;
            }
            break;
        case OMX_IndexExtEnableErrorReport:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtEnableErrorReport");
                OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT* pParam = (OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT*)pComponentParameterStructure;
                if (IsStructVersionValid<OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT>(pParam, sizeof(OMX_VIDEO_CONFIG_INTEL_ERROR_REPORT), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexExtEnableErrorReport), pParam->nPortIndex))
                    {
                        m_bErrorReportingEnabled = pParam->bEnable;
                        omx_res = OMX_ErrorNone;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamIntelAVCDecodeSettings:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamIntelAVCDecodeSettings");
                OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS* pParam = (OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS>(pParam, sizeof(OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexParamIntelAVCDecodeSettings), pParam->nPortIndex))
                    {
                        if (!bIncorrectState || !IsPortEnabled(pParam->nPortIndex))
                        {
                            MfxOmxVideoPortData* video_port = (MfxOmxVideoPortData*)(m_pPorts[pParam->nPortIndex]);
                            video_port->m_intel_avc_decode_settings = *pParam;
                            MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS(video_port->m_intel_avc_decode_settings);
                        }
                        else omx_res = OMX_ErrorInvalidState;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexIntelEnableSFC:
            {
                MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEnableSFC");
                OMX_VIDEO_PARAM_SFC* pParam = (OMX_VIDEO_PARAM_SFC*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_VIDEO_PARAM_SFC>(pParam, sizeof(OMX_VIDEO_PARAM_SFC), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEnableSFC), pParam->nPortIndex))
                    {
                        if (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId)
                        {
                            m_bEnableVP = pParam->bEnableSFC;

                            m_decVideoProc.In.CropX = 0;
                            m_decVideoProc.In.CropY = 0;
                            m_decVideoProc.In.CropW = m_MfxVideoParams.mfx.FrameInfo.CropW;
                            m_decVideoProc.In.CropH = m_MfxVideoParams.mfx.FrameInfo.CropH;


                            m_decVideoProc.Out.FourCC = m_MfxVideoParams.mfx.FrameInfo.FourCC;
                            m_decVideoProc.Out.ChromaFormat = m_MfxVideoParams.mfx.FrameInfo.ChromaFormat;
                            m_decVideoProc.Out.Width = pParam->nOutputWidth;
                            m_decVideoProc.Out.Height = pParam->nOutputHeight;
                            m_decVideoProc.Out.CropX = 0;
                            m_decVideoProc.Out.CropY = 0;
                            m_decVideoProc.Out.CropW = pParam->nOutputWidth;
                            m_decVideoProc.Out.CropH = pParam->nOutputHeight;

                            omx_res = OMX_ErrorNone;
                        }
                        else
                            omx_res = OMX_ErrorUnsupportedSetting;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexIntelDecodedOrder:
            {
                MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelDecodedOrder");
                OMX_CONFIG_DECODEDORDER *pParam = static_cast<OMX_CONFIG_DECODEDORDER *>(pComponentParameterStructure);

                if (IsStructVersionValid<OMX_CONFIG_DECODEDORDER>(pParam, sizeof(OMX_CONFIG_DECODEDORDER), OMX_VERSION))
                {
                    if (MFX_CODEC_AVC  == m_MfxVideoParams.mfx.CodecId ||
                        MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId)
                    {
                        m_MfxVideoParams.mfx.DecodedOrder = (OMX_TRUE == pParam->bDecodedOrder) ? 1 : 0;
                        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParameter(MfxOmx_IndexIntelDecodedOrder) DecodedOrder = %d", m_MfxVideoParams.mfx.DecodedOrder);

                        omx_res = OMX_ErrorNone;
                    }
                    else omx_res = OMX_ErrorUnsupportedSetting;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("Unknown nParamIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }

    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::GetConfig(
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::GetConfig(nIndex, pComponentConfigStructure);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (static_cast<int> (nIndex))
        {
        case OMX_IndexConfigCommonOutputCrop:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigCommonOutputCrop");
                OMX_CONFIG_RECTTYPE* pParam = (OMX_CONFIG_RECTTYPE*)pComponentConfigStructure;

                // NOTE: at the moment stagefright does not set version for this structure
                //if (IsStructVersionValid<OMX_CONFIG_RECTTYPE>(pParam, sizeof(OMX_CONFIG_RECTTYPE), OMX_VERSION))
                {
                    pParam->nLeft   = m_Crops.CropX;
                    pParam->nTop    = m_Crops.CropY;
                    pParam->nWidth  = m_Crops.CropW;
                    pParam->nHeight = m_Crops.CropH;

                    MFX_OMX_AUTO_TRACE_I32(pParam->nLeft);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nTop);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nWidth);
                    MFX_OMX_AUTO_TRACE_I32(pParam->nHeight);
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "GetConfig(ConfigCommonOutputCrop) nLeft %d, nTop %d, nWidth %d, nHeight %d",
                                        pParam->nLeft, pParam->nTop, pParam->nWidth, pParam->nHeight);

                    omx_res = OMX_ErrorNone;
                }
                //else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexExtDecoderBufferHandle:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtDecoderBufferHandle");
                OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE* pParam = (OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE*)pComponentConfigStructure;
                if (IsStructVersionValid<OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE>(pParam, sizeof(OMX_VIDEO_CONFIG_INTEL_DECODER_BUFFER_HANDLE), OMX_VERSION))
                {
                    if (IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexExtDecoderBufferHandle), pParam->nPortIndex))
                    {
                        if (pParam->nIndex < m_pOutPortDef->nBufferCountActual)
                        {
                            pParam->pNativeHandle = m_pBufferHeaders[pParam->nIndex]->pBuffer;
                            omx_res = OMX_ErrorNone;
                        }
                        else
                            omx_res = OMX_ErrorBadParameter;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGoogleDescribeColorAspects:
            {
                MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexGoogleDescribeColorAspects");

                android::DescribeColorAspectsParams *colorAspectsParams =
                    static_cast<android::DescribeColorAspectsParams *>(pComponentConfigStructure);

                m_colorAspects.GetOutputColorAspects(colorAspectsParams->sAspects);

                // The framework uses OMX_GetConfig during idle and executing state
                // to get guidance for the dataspace to set for given color aspects,
                // by setting bRequestingDataSpace to OMX_TRUE (optional request).
                // The component SHALL return OMX_ErrorUnsupportedSettings
                // if it does not support this request.
                if (colorAspectsParams->bRequestingDataSpace)
                {
                    omx_res = OMX_ErrorUnsupportedSetting;
                }

                omx_res = OMX_ErrorNone;
                break;
            }
#ifdef HEVC10HDR_SUPPORT
        // NOTE: may be used for sending HDR SEI
        case MfxOmx_IndexGoogleDescribeHDRStaticInfo:
            {
                MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexGoogleDescribeHDRStaticInfo");
                MFX_OMX_AUTO_TRACE_I32(m_bIsSetHDRSEI);

                if (m_bIsSetHDRSEI)
                {
                    android::DescribeHDRStaticInfoParams *seiHDRStaticInfoParams =
                           static_cast<android::DescribeHDRStaticInfoParams *>(pComponentConfigStructure);

                    seiHDRStaticInfoParams->sInfo = m_SeiHDRStaticInfo;
                }
                omx_res = OMX_ErrorNone;
                break;
            }
#endif
        default:
            MFX_OMX_AUTO_TRACE_MSG("Unknown nIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::SetConfig(
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::SetConfig(nIndex, pComponentConfigStructure);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (static_cast<int>(nIndex))
        {
        case MfxOmx_IndexGoogleDescribeColorAspects:
        {
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexGoogleDescribeColorAspects");

            const android::DescribeColorAspectsParams *colorAspectsParams =
                static_cast<const android::DescribeColorAspectsParams *>(pComponentConfigStructure);

            m_colorAspects.SetFrameworkColorAspects(colorAspectsParams->sAspects);

            omx_res = OMX_ErrorNone;
            break;
        }
#ifdef HEVC10HDR_SUPPORT
        case MfxOmx_IndexGoogleDescribeHDRStaticInfo:
        {
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexGoogleDescribeHDRStaticInfo");

            android::DescribeHDRStaticInfoParams *seiHDRStaticInfoParams =
                static_cast<android::DescribeHDRStaticInfoParams *>(pComponentConfigStructure);

            m_SeiHDRStaticInfo = seiHDRStaticInfoParams->sInfo;
            m_bIsSetHDRSEI = true;

            omx_res = OMX_ErrorNone;
            break;
        }
#endif
        default:
            MFX_OMX_AUTO_TRACE_MSG("Unknown nIndex");
            omx_res = OMX_ErrorNone; // OMX_ErrorUnsupportedIndex; TODO: set for backward compatibility
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::BufferReleased(
    MfxOmxPoolId id,
    OMX_BUFFERHEADERTYPE* pBuffer,
    mfxStatus error)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(error);
    if (id == m_pOmxBitstream->GetPoolId())
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EmptyBufferDone(%p)", pBuffer);
        m_pCallbacks->EmptyBufferDone(m_self, m_pAppData, pBuffer);
    }
    else if (id == m_pSurfaces->GetPoolId())
    {
        if (m_bOnFlySurfacesAllocation && m_bANWBufferInMetaData)
        {
            int fenceFd = -1;
            if (pBuffer->nAllocLen >= sizeof(android::VideoNativeMetadata))
            {
                android::VideoNativeMetadata &nativeMeta = *(android::VideoNativeMetadata *)(pBuffer->pBuffer);
                if (nativeMeta.eType == android::kMetadataBufferTypeANWBuffer)
                {
                    fenceFd = nativeMeta.nFenceFd;
                }
            }
            if (fenceFd != -1 && (pBuffer->nFlags & OMX_BUFFERFLAG_EOS) == 0)
                pBuffer->nFilledLen = pBuffer->nAllocLen;
        }
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "FillBufferDone(%p) nTimeStamp %lld, nFilledLen %d, nFlags 0x%x",
                            pBuffer, pBuffer->nTimeStamp, pBuffer->nFilledLen, pBuffer->nFlags);
        m_pCallbacks->FillBufferDone(m_self, m_pAppData, pBuffer);
    }
}

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE MfxOmxVdecComponent::Validate(
    MfxOmxInputConfig& config,
    OMX_INDEXTYPE idx,
    const T* omxparams,
    OMX_U32 omxversion)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(config);
    if (!omxparams)
    {
        return OMX_ErrorBadParameter;
    }
    if (!IsStructVersionValid<T>(omxparams, sizeof(T), omxversion))
    {
        return OMX_ErrorVersionMismatch;
    }
    if (!IsIndexValid(static_cast<OMX_INDEXTYPE>(idx), omxparams->nPortIndex))
    {
        return OMX_ErrorBadPortIndex;
    }

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::ValidateConfig(
        OMX_U32 kind,
        OMX_INDEXTYPE nIndex,
        OMX_PTR pConfig,
        MfxOmxInputConfig & config)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorUnsupportedIndex;

    switch (static_cast<int> (nIndex))
    {
    case MFX_OMX_IndexConfigPriority:
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_IndexConfigPriority");
        if (kind != eSetConfig) break;
        omx_res = Validate(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_PRIORITY*>(pConfig));

            omx_res = OMX_ErrorNone; // Hide unsupported index
        break;
    case MFX_OMX_IndexConfigOperatingRate:
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_IndexConfigOperatingRate");
        if (kind != eSetConfig) break;
        omx_res = Validate(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_OPERATION_RATE*>(pConfig));

            omx_res = OMX_ErrorNone; // Hide unsupported index
        break;
    default:
        omx_res = OMX_ErrorUnsupportedIndex;
        break;
    };

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE MfxOmxVdecComponent::mfx2omx(
    OMX_INDEXTYPE idx,
    T* omxparams,
    const mfxVideoParam& mfxparams,
    OMX_U32 omxversion)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (!omxparams)
    {
        return OMX_ErrorBadParameter;
    }
    if (!IsStructVersionValid<T>(omxparams, sizeof(T), omxversion))
    {
        return OMX_ErrorVersionMismatch;
    }
    if (!IsIndexValid(static_cast<OMX_INDEXTYPE>(idx), omxparams->nPortIndex))
    {
        return OMX_ErrorBadPortIndex;
    }
    return mfx2omx_config(*omxparams, mfxparams);
}

/*------------------------------------------------------------------------------*/

/* NOTE:
 * - Possibility of the state transition is already validated on the enter
 * to this function.
 */
bool MfxOmxVdecComponent::IsStateTransitionReady(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    bool bReady = false;

    MFX_OMX_AUTO_TRACE_I32(m_state);
    MFX_OMX_AUTO_TRACE_I32(m_state_to_set);
    if (m_state != m_state_to_set)
    {
        switch (m_state)
        {
        case OMX_StateLoaded:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateLoaded");
            if ((OMX_StateIdle == m_state_to_set) || (OMX_StateWaitForResources == m_state_to_set))
            {
                bReady = ((OMX_TRUE == m_pInPortDef->bPopulated) || (OMX_FALSE == m_pInPortDef->bEnabled)) &&
                         ((OMX_TRUE == m_pOutPortDef->bPopulated) || (OMX_FALSE == m_pOutPortDef->bEnabled));
            }
            else bReady = true;
            break;
        case OMX_StateIdle:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateIdle");
            if (OMX_StateLoaded == m_state_to_set)
            {
                bReady = !(m_pInPortInfo->nCurrentBuffersCount) && !(m_pOutPortInfo->nCurrentBuffersCount);
            }
            else bReady = true;
            break;
        case OMX_StateExecuting:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateExecuting");
            bReady = true;
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("Unhandled state: possible BUG");
            break;
        };
    }
    // disabling port(s)
    else if (m_pInPortInfo->bDisable && m_pOutPortInfo->bDisable)
    {
        bReady = !(m_pInPortInfo->nCurrentBuffersCount) && !(m_pOutPortInfo->nCurrentBuffersCount);
    }
    else if (m_pInPortInfo->bDisable)
    {
        bReady = !(m_pInPortInfo->nCurrentBuffersCount);
    }
    else if (m_pOutPortInfo->bDisable)
    {
        bReady = !(m_pOutPortInfo->nCurrentBuffersCount);
    }
    // enabling port(s)
    else if (m_pInPortInfo->bEnable && m_pOutPortInfo->bEnable)
    {
        bReady = (OMX_TRUE == m_pInPortDef->bPopulated) && (OMX_TRUE == m_pOutPortDef->bPopulated);
    }
    else if (m_pInPortInfo->bEnable)
    {
        bReady = (OMX_TRUE == m_pInPortDef->bPopulated);
    }
    else if (m_pOutPortInfo->bEnable)
    {
        bReady = (OMX_TRUE == m_pOutPortDef->bPopulated);
    }
    MFX_OMX_AUTO_TRACE_I32(bReady);
    return bReady;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::CommandStateSet(OMX_STATETYPE new_state)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "CommandStateSet - current state %d, new_state %d", m_state, new_state);
    MFX_OMX_AUTO_TRACE_I32(m_state);
    MFX_OMX_AUTO_TRACE_I32(new_state);
    if (new_state == m_state)
    {
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorSameState, 0 , NULL);
    }
    else
    {
        switch (new_state)
        {
        case OMX_StateInvalid:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateInvalid");
            m_state = OMX_StateInvalid;
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorInvalidState, 0, NULL);
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            break;
        case OMX_StateLoaded:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateLoaded");
            if ((OMX_StateIdle == m_state) || (OMX_StateWaitForResources == m_state))
            {
                m_state_to_set = OMX_StateLoaded;
                // closing codec
                CloseCodec();
                // awaiting while all buffers will be freed
                if (!IsStateTransitionReady())
                {
                    MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
                    lock.Unlock();
                    m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
                    lock.Lock();
                }
                m_state_to_set = m_state = OMX_StateLoaded;
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            }
            else
            {
                MFX_OMX_LOG_ERROR("Error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        case OMX_StateIdle:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateIdle");
            if (OMX_StateInvalid == m_state)
            {
                MFX_OMX_LOG_ERROR("Error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            else
            {
                m_state_to_set = OMX_StateIdle;
                if (OMX_StateExecuting == m_state)
                { // resetting codec
                    // TODO: loop is needed here: while (MFX_ERR_NONE != ResetCodec()) ;
                    ResetCodec();
                }
                if (!IsStateTransitionReady())
                {
                    MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
                    lock.Unlock();
                    m_bTransition = true;
                    m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
                    lock.Lock();
                }
                m_state_to_set = m_state = OMX_StateIdle;
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            }
            break;
        case OMX_StateExecuting:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateExecuting");
            if ((OMX_StateIdle == m_state) || (OMX_StatePause == m_state))
            {
                m_state_to_set = m_state = OMX_StateExecuting;
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            }
            else
            {
                MFX_OMX_LOG_ERROR("Error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        case OMX_StatePause:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StatePause");
            if ((OMX_StateIdle == m_state) || (OMX_StateExecuting == m_state))
            {
                m_state_to_set = m_state = OMX_StatePause;
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            }
            else
            {
                MFX_OMX_LOG_ERROR("Error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        case OMX_StateWaitForResources:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateWaitForResources");
            if (OMX_StateLoaded == m_state)
            {
                m_state_to_set = m_state = OMX_StateWaitForResources;
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandStateSet, m_state, NULL);
            }
            else
            {
                MFX_OMX_LOG_ERROR("Error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("Switch to unhandled state: possible BUG");
            break;
        };
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::CommandPortDisable(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    if ((MFX_OMX_INPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling input port");
        MFX_OMX_AUTO_TRACE("Disabling input port");
        // releasing all buffers
        m_pOmxBitstream->Reset();
        // disabling port
        m_pInPortDef->bEnabled = OMX_FALSE;
        m_pInPortDef->bPopulated = OMX_FALSE;
        m_pInPortInfo->bDisable = true;
        if (!IsStateTransitionReady())
        {
            MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
            lock.Unlock();
            m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
            lock.Lock();
        }
        m_pInPortInfo->bDisable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortDisable, m_pInPortInfo->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling input port completed");
    }
    if ((MFX_OMX_OUTPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling output port");
        MFX_OMX_AUTO_TRACE("Disabling output port");
        // swap pBufInfo for each buffer before closing surface pool because after that we lose swap information and get error in freeBuffer
        if (m_bANWBufferInMetaData)
        {
            for (mfxU32 i = 0; i < m_nBufferHeadersSize; ++i)
            {
                if (m_pBufferHeaders[i])
                {
                    m_pSurfaces->SwapBufferInfo(m_pBufferHeaders[i]);
                }
            }
        }
        // releasing all buffers
        m_pSurfaces->Close();

        MFX_OMX_AUTO_TRACE_MSG("free surfaces");
        if (m_pDevice && !m_bUseSystemMemory)
        {
            MfxOmxVaapiFrameAllocator* pvaAllocator = (MfxOmxVaapiFrameAllocator*)m_pDevice->GetFrameAllocator();
            if(pvaAllocator)
            {
                pvaAllocator->FreeSurfaces();
            }
            MFX_OMX_ZERO_MEMORY(m_AllocResponse);
        }
        // disabling port
        m_pOutPortDef->bEnabled = OMX_FALSE;
        m_pOutPortDef->bPopulated = OMX_FALSE;
        m_pOutPortInfo->bDisable = true;
        if (!IsStateTransitionReady())
        {
            MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
            lock.Unlock();
            m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
            lock.Lock();
        }
        MFX_OMX_FREE(m_pBufferHeaders);
        m_nBufferHeadersSize = 0;

        m_pOutPortInfo->bDisable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortDisable, m_pOutPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling output port completed");
    }
    else omx_res = OMX_ErrorBadPortIndex;
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::CommandPortEnable(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    if ((MFX_OMX_INPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Enabling input port");
        MFX_OMX_AUTO_TRACE("Enabling input port");
        m_pInPortInfo->bEnable = true;
        if (!IsStateTransitionReady())
        {
            MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
            lock.Unlock();
            m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
            lock.Lock();
        }
        // enabling port
        m_pInPortDef->bEnabled = OMX_TRUE;
        m_pInPortInfo->bEnable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortEnable, m_pInPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Enabling input port completed");
    }
    if ((MFX_OMX_OUTPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Enabling output port");
        MFX_OMX_AUTO_TRACE("Enabling output port");
        m_pOutPortInfo->bEnable = true;
        if (!IsStateTransitionReady())
        {
            MFX_OMX_AUTO_TRACE("Awaiting for m_pStateTransitionEvent");
            lock.Unlock();
            m_bTransition = true;
            m_pStateTransitionEvent->Wait(); // awaiting when state transition will be possible
            lock.Lock();
        }
        // enabling port
        m_pOutPortDef->bEnabled = OMX_TRUE;
        if (m_bChangeOutputPortSettings)
        {
            m_InitState = MFX_INIT_DECODER;
            m_bChangeOutputPortSettings = false;
        }
        m_Crops = m_MfxVideoParams.mfx.FrameInfo;
        m_pOutPortInfo->bEnable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortEnable, m_pOutPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Enabling output port completed");
    }
    else omx_res = OMX_ErrorBadPortIndex;
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVdecComponent::CommandFlush(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxStatus mfx_sts = MFX_ERR_NONE;

    m_bFlush = true;
    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    if ((MFX_OMX_INPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush input buffers");
        MFX_OMX_AUTO_TRACE("Flush input buffers");
        if (MFX_INIT_DECODE_HEADER == m_InitState)
        {
            OMX_BUFFERHEADERTYPE* pBuffer = NULL;
            if (NULL != (pBuffer = m_pOmxBitstream->GetBuffer()))
            {
                BitstreamLoader loader(m_pOmxBitstream);
                mfx_sts = loader.LoadBuffer(pBuffer);
                if ((MFX_ERR_NONE != mfx_sts) && (MFX_ERR_NULL_PTR != mfx_sts))
                {
                    MFX_OMX_LOG_ERROR("LoadBuffer failed");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "CommandFlush : LoadBuffer failed : mfx_sts = %d", mfx_sts);
                    MFX_OMX_AUTO_TRACE_I32(mfx_sts);
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(mfx_sts), 0 , NULL);
                }
            }
        }
        // releasing all buffers
        mfx_sts = ResetInput();
        if (MFX_ERR_NONE == mfx_sts)
        {
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandFlush, MFX_OMX_INPUT_PORT_INDEX, NULL);
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush input buffers completed");
        }
        else
        {
            MFX_OMX_LOG_ERROR("Flush input buffers failed");
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "CommandFlush : Flush input buffers failed : mfx_sts = %d", mfx_sts);
            MFX_OMX_AUTO_TRACE_I32(mfx_sts);
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(mfx_sts), 0 , NULL);
        }
    }
    if ((MFX_OMX_OUTPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush output buffers");
        MFX_OMX_AUTO_TRACE("Flush output buffers");

        mfx_sts = ResetOutput();
        if (MFX_ERR_NONE == mfx_sts)
        {
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandFlush, MFX_OMX_OUTPUT_PORT_INDEX, NULL);
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush output buffers completed");
        }
        else
        {
            MFX_OMX_LOG_ERROR("Flush output buffers failed");
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "CommandFlush : Flush output buffers failed : mfx_sts = %d", mfx_sts);
            MFX_OMX_AUTO_TRACE_I32(mfx_sts);
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(mfx_sts), 0 , NULL);
        }
    }
    if ((MFX_OMX_INPUT_PORT_INDEX != nPortIndex) && (MFX_OMX_OUTPUT_PORT_INDEX != nPortIndex) && (OMX_ALL != nPortIndex))
        omx_res = OMX_ErrorBadPortIndex;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::MainThread(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxInputData input;
    MfxOmxCommandData& command = input.command;
    mfxStatus mfx_sts = MFX_ERR_NONE;

    while (1)
    {
        m_pCommandsSemaphore->Wait();

        if (m_bDestroy) break;

        MFX_OMX_AUTO_TRACE("Main Thread Loop iteration");

        input.type = MfxOmxInputData::MfxOmx_InputData_None;
        m_input_queue.Get(&input);
        MFX_OMX_AUTO_TRACE_I32(input.type);

        if (input.type == MfxOmxInputData::MfxOmx_InputData_Command)
        {
            MFX_OMX_AUTO_TRACE("Got new command");
            MFX_OMX_AUTO_TRACE_I32(command.m_command);

            MFX_OMX_AUTO_TRACE_MSG("m_pAllSyncOpFinished->Wait()");
            m_pAllSyncOpFinished->Wait();

            switch (command.m_command)
            {
            case OMX_CommandStateSet:
                MFX_OMX_AUTO_TRACE_MSG("OMX_CommandStateSet");
                CommandStateSet(command.m_new_state);
                break;
            case OMX_CommandPortDisable:
                MFX_OMX_AUTO_TRACE_MSG("OMX_CommandPortDisable");
                CommandPortDisable(command.m_port_number);
                break;
            case OMX_CommandPortEnable:
                MFX_OMX_AUTO_TRACE_MSG("OMX_CommandPortEnable");
                CommandPortEnable(command.m_port_number);
                break;
            case OMX_CommandFlush:
                MFX_OMX_AUTO_TRACE_MSG("OMX_CommandFlush");
                CommandFlush(command.m_port_number);
                break;
            default:
                MFX_OMX_AUTO_TRACE_MSG("Command ignored");
                break;
            };
        }

        if ((OMX_StateExecuting == m_state) && (MFX_ERR_NONE == m_Error) && ArePortsEnabled(OMX_ALL) && CanDecode())
        {
            MFX_OMX_AUTO_TRACE_MSG("Trying to decode");

            mfx_sts = MFX_ERR_NONE;

            OMX_BUFFERHEADERTYPE* pBuffer = NULL;

            if (MFX_INIT_DECODER == m_InitState)
            {
                mfx_sts = InitCodec();
                if (MFX_ERR_NONE != mfx_sts) m_Error = mfx_sts;

                m_bEosHandlingStarted = false;
            }

            if ((MFX_ERR_NONE == m_Error) && !m_bEosHandlingStarted)
            {
                BitstreamLoader loader(m_pOmxBitstream);

                while (((MFX_ERR_NONE == mfx_sts) || (MFX_ERR_MORE_DATA == mfx_sts)) && CanDecode())
                {
                    m_pBitstream = m_pOmxBitstream->GetFrameConstructor()->GetMfxBitstream();

                    if (MFX_INIT_COMPLETED != m_InitState)
                    {
                        mfx_sts = InitCodec();

                        // When we receive the bitstream with incorrect sps/pps, the msdk try to find the other correct sps/pps.
                        // In case, if the sps/pps has not found yet, but EOS has been reached then we need to return the eos output buffer
                        // There is CTS test to check this behavior
                        if(MFX_ERR_MORE_DATA == mfx_sts && m_pOmxBitstream->GetFrameConstructor()->WasEosReached())
                        {
                            mfxFrameSurface1 *pWorkSurface = m_pSurfaces->GetBuffer();
                            if (pWorkSurface)
                            {
                                m_pSurfaces->QueueBufferForSending(pWorkSurface, NULL);
                                m_bEosHandlingFinished = true;
                                m_pAsyncSemaphore->Post();
                            }
                        }

                        if (MFX_INIT_QUERY_IO_SURF == m_InitState)
                        {
                            if (!m_bChangeOutputPortSettings)
                                m_InitState = MFX_INIT_DECODER;

                            break; // need to handle m_commands
                        }
                    }

                    if ((MFX_ERR_NONE == mfx_sts) && !m_bChangeOutputPortSettings)
                    {
                        if (m_bFlush)
                        {
                            m_bFlush = false;
                            if (IsResolutionChanged())
                            {
                                mfx_sts = ReinitCodec();
                                if (MFX_ERR_NONE != mfx_sts)
                                {
                                    MFX_OMX_AUTO_TRACE("Failed to reinit codec");
                                    m_Error = mfx_sts;
                                }
                                break; // need to handle m_commands
                            }
                        }

                        mfx_sts = DecodeFrame();
                    }

                    m_pOmxBitstream->GetFrameConstructor()->Sync();

                    if (!m_bChangeOutputPortSettings && MFX_ERR_MORE_DATA == mfx_sts)
                    {
                        pBuffer = m_pOmxBitstream->GetBuffer();
                        if (pBuffer)
                        {
                            mfx_sts = loader.LoadBuffer(pBuffer);

                            // MFX_ERR_NULL_PTR is a valid status, we need to continue Decoding/Initialization,
                            // Without this we will go outside the "while" loop
                            if (MFX_ERR_NULL_PTR == mfx_sts) mfx_sts = MFX_ERR_NONE;

                            if (MFX_ERR_NONE != mfx_sts)
                            {
                                m_Error = mfx_sts;
                                break;
                            }
                        }
                        else
                            break;
                    }
                }

                if ((m_pOmxBitstream->GetFrameConstructor()->WasEosReached() &&
                     m_pOmxBitstream->GetFrameConstructor()->GetMfxBitstream()->DataLength == 0) || m_bReinit)
                {
                    m_bEosHandlingStarted = true;
                }

                if (MFX_ERR_NONE != mfx_sts &&
                    MFX_ERR_MORE_DATA != mfx_sts && MFX_ERR_MORE_SURFACE != mfx_sts &&
                    MFX_ERR_NULL_PTR != mfx_sts && MFX_ERR_INCOMPATIBLE_VIDEO_PARAM != mfx_sts)
                {
                    m_Error = mfx_sts;
                }
            }

            // and finally we handle EOS / DRC if we reached it
            if ((MFX_ERR_NONE == m_Error) && !m_bChangeOutputPortSettings && m_bEosHandlingStarted && !m_bEosHandlingFinished)
            {
                MFX_OMX_AUTO_TRACE("Retrieving buffered frames");
                if (m_pOmxBitstream->GetFrameConstructor()->WasEosReached() || m_bReinit)
                {
                    m_pBitstream = NULL;

                    mfx_sts = DecodeFrame();
                    m_pOmxBitstream->GetFrameConstructor()->Sync();
                    if ((MFX_ERR_NONE != mfx_sts) && (MFX_ERR_MORE_SURFACE != mfx_sts))
                    {
                        if (!m_bReinit)
                        {
                            mfxFrameSurface1 *pWorkSurface = m_pSurfaces->GetBuffer();

                            if (pWorkSurface)
                            {
                                mfx_sts = m_pSurfaces->QueueBufferForSending(pWorkSurface, NULL);

                                m_bEosHandlingFinished = true;

                                m_pAsyncSemaphore->Post();
                            }
                        }
                        else
                        {
                            m_pAllSyncOpFinished->Wait();
                            MFX_OMX_AUTO_TRACE_MSG("Buffered frames flush is finished. Resetting decoder");
                            m_bEosHandlingStarted = m_bEosHandlingFinished = false;

                            mfx_sts = ReinitCodec();
                            if (MFX_ERR_MORE_DATA == mfx_sts)
                            {
                                OMX_BUFFERHEADERTYPE* pBuffer = m_pOmxBitstream->GetBuffer();
                                if (pBuffer)
                                {
                                    BitstreamLoader loader(m_pOmxBitstream);
                                    mfx_sts = loader.LoadBuffer(pBuffer);
                                    if ((MFX_ERR_NONE != mfx_sts) && (MFX_ERR_NULL_PTR != mfx_sts))
                                    {
                                        MFX_OMX_AUTO_TRACE("LoadBuffer failed");
                                        m_Error = mfx_sts;
                                    }
                                }

                                if (m_pOmxBitstream->GetFrameConstructor()->WasEosReached())
                                {
                                    // ReinitCodec() returned MFX_ERR_MORE_DATA (only from DecodeHeader) and we got the EOS.
                                    // So, we cannot decode this part of bitstream because it is corrupted.
                                    //
                                    // This means that we met at the bitstream new SPS and trying to reinit codec
                                    // (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM from DecodeFrameAsync),
                                    // but cannot parse header data to decode bitstream (for example, cannot find new PPS),
                                    // and we got from framework all bitstream data.
                                    //
                                    // Example of the problem (bug_38487564) :
                                    // ------------------------------------------------------------------------
                                    // <old SPS/SPS and data> | new SPS | <some data without new PPS> ... |EOS|
                                    // ------------------------------------------------------------------------
                                    //                        ^                                             ^
                                    //                        |                                             |
                                    //             MSDK stay at the point on new SPS                   EOS flag from
                                    //             and trying to find related PPS                        framework

                                    MFX_OMX_LOG_ERROR("Sending ErrorEvent to OMAX - OMX_ErrorStreamCorrupt");
                                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Sending ErrorEvent to OMAX - OMX_ErrorStreamCorrupt");
                                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, OMX_ErrorStreamCorrupt, 0 , NULL);
                                }
                            }
                            else if (MFX_ERR_NONE != mfx_sts)
                            {
                                MFX_OMX_AUTO_TRACE("ReinitCodec failed");
                                m_Error = mfx_sts;
                            }
                        }
                    }
                }
            }
            if (MFX_ERR_NONE != m_Error)
            {
                MFX_OMX_LOG_ERROR("Sending ErrorEvent to OMAX client because of error in component");
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "MainThread : m_Error = %d", m_Error);
                MFX_OMX_AUTO_TRACE_I32(m_Error);
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(m_Error), 0 , NULL);
            }
        }
    }
}

/*------------------------------------------------------------------------------*/

mfxU16 MfxOmxVdecComponent::GetAsyncDepth(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxU16 asyncDepth;
    if ((MFX_IMPL_HARDWARE == MFX_IMPL_BASETYPE(m_Implementation)) &&
        ((MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId) ||
         (MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId) ||
         (MFX_CODEC_VP8 == m_MfxVideoParams.mfx.CodecId) ||
         (MFX_CODEC_VP9 == m_MfxVideoParams.mfx.CodecId)))
        asyncDepth = 1;
    else
        asyncDepth = 0;

    MFX_OMX_AUTO_TRACE_I32(asyncDepth);
    return asyncDepth;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::InitCodec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (MFX_INIT_DECODE_HEADER == m_InitState)
    {
        MFX_OMX_AUTO_TRACE("MFX_INIT_DECODE_HEADER");

        // saving parameters
        mfxVideoParam oldParams = m_MfxVideoParams;

        m_extBuffers.push_back(reinterpret_cast<mfxExtBuffer*>(&m_signalInfo));
        m_MfxVideoParams.NumExtParam = m_extBuffers.size();
        m_MfxVideoParams.ExtParam = &m_extBuffers.front();

        // decoding header
        mfx_res = m_pDEC->DecodeHeader(m_pBitstream, &m_MfxVideoParams);

        m_extBuffers.pop_back();
        m_MfxVideoParams.NumExtParam = oldParams.NumExtParam;
        m_MfxVideoParams.ExtParam = oldParams.ExtParam;

        // valid cases are:
        // MFX_ERR_NONE - header decoded, initialization can be performed
        // MFX_ERR_MORE_DATA - header not decoded, need provide more data
        if (MFX_ERR_NULL_PTR == mfx_res) mfx_res = MFX_ERR_MORE_DATA;
        if (MFX_ERR_NONE == mfx_res)
        {
            // check SPS for detect interlaced content
            bool bInterlaced;
            if (m_pOmxBitstream->GetFrameConstructor()->IsSetInterlaceFlag(&bInterlaced))
                m_bInterlaced = bInterlaced;
            else
                MFX_OMX_AUTO_TRACE_MSG("WARNING: m_bInterlaced was not set");
            MFX_OMX_AUTO_TRACE_I32(m_bInterlaced);

            if ((MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId) && (MFX_PROFILE_HEVC_MAIN10 == m_MfxVideoParams.mfx.CodecProfile))
            {
                if (m_bUseSystemMemory)
                    mfx_res = MFX_ERR_UNSUPPORTED;
                else
                    m_MfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_P010;
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            if (m_MfxVideoParams.mfx.FrameInfo.Width > 8192 || m_MfxVideoParams.mfx.FrameInfo.Height > 8192)
            {
                mfx_res = MFX_ERR_UNSUPPORTED;
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            m_colorAspects.UpdateBitsreamColorAspects(m_signalInfo);

#ifdef HEVC10HDR_SUPPORT
            UpdateHdrStaticInfo();
#endif

            // restoring some parameters
            if (!m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN || !m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD)
            {
                m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN = oldParams.mfx.FrameInfo.FrameRateExtN;
                m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD = oldParams.mfx.FrameInfo.FrameRateExtD;
            }
            if (!m_MfxVideoParams.mfx.FrameInfo.AspectRatioW || !m_MfxVideoParams.mfx.FrameInfo.AspectRatioH)
            {
                m_MfxVideoParams.mfx.FrameInfo.AspectRatioW = oldParams.mfx.FrameInfo.AspectRatioW;
                m_MfxVideoParams.mfx.FrameInfo.AspectRatioH = oldParams.mfx.FrameInfo.AspectRatioH;
            }

            if (m_MfxVideoParams.mfx.FrameInfo.Width > m_nMaxFrameWidth || !m_bLegacyAdaptivePlayback || m_bOnFlySurfacesAllocation)
                m_nMaxFrameWidth = m_MfxVideoParams.mfx.FrameInfo.Width;

            if (m_MfxVideoParams.mfx.FrameInfo.Height > m_nMaxFrameHeight || !m_bLegacyAdaptivePlayback || m_bOnFlySurfacesAllocation)
                m_nMaxFrameHeight = m_MfxVideoParams.mfx.FrameInfo.Height;

            m_pOmxBitstream->InitFrameConstructor(m_MfxVideoParams.mfx.CodecProfile, m_MfxVideoParams.mfx.FrameInfo);
            m_InitState = MFX_INIT_QUERY_IO_SURF;

            if (m_bEnableVP)
            {
                m_decVideoProc.In.CropX = m_MfxVideoParams.mfx.FrameInfo.CropX;
                m_decVideoProc.In.CropY = m_MfxVideoParams.mfx.FrameInfo.CropY;
                m_decVideoProc.In.CropW = m_MfxVideoParams.mfx.FrameInfo.CropW;
                m_decVideoProc.In.CropH = m_MfxVideoParams.mfx.FrameInfo.CropH;

                m_decVideoProc.Out.FourCC = m_MfxVideoParams.mfx.FrameInfo.FourCC;
                m_decVideoProc.Out.ChromaFormat = m_MfxVideoParams.mfx.FrameInfo.ChromaFormat;
                m_decVideoProc.Out.CropX = 0;
                m_decVideoProc.Out.CropY = 0;
                m_decVideoProc.Out.CropW = m_decVideoProc.Out.Width;
                m_decVideoProc.Out.CropH = m_decVideoProc.Out.Height;
            }
        }
        else
        {
            // copying parameters back on MFX_ERR_MORE_DATA and errors
            m_MfxVideoParams = oldParams;
        }
     }

     if ((MFX_INIT_QUERY_IO_SURF == m_InitState) && (MFX_ERR_NONE == mfx_res))
     {
        MFX_OMX_AUTO_TRACE("MFX_INIT_QUERY_IO_SURF");

        m_MfxVideoParams.AsyncDepth = GetAsyncDepth();
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "AsyncDepth %d", m_MfxVideoParams.AsyncDepth);

        m_MfxVideoParams.IOPattern = (mfxU16)((m_pDevice && !m_bUseSystemMemory) ? MFX_IOPATTERN_OUT_VIDEO_MEMORY : MFX_IOPATTERN_OUT_SYSTEM_MEMORY);

        if (MFX_ERR_NONE == mfx_res)
        {
            if (m_bEnableVP) m_extBuffers.push_back(reinterpret_cast<mfxExtBuffer*>(&m_decVideoProc));
            if (!m_extBuffers.empty())
            {
                m_MfxVideoParams.NumExtParam = m_extBuffers.size();
                m_MfxVideoParams.ExtParam = &m_extBuffers.front();
            }

            mfxFrameAllocRequest request;
            MFX_OMX_ZERO_MEMORY(request);

            mfx_res = m_pDEC->QueryIOSurf(&m_MfxVideoParams, &request);
            if (MFX_WRN_PARTIAL_ACCELERATION == mfx_res)
            {
                MFX_OMX_LOG_INFO("MFX_WRN_PARTIAL_ACCELERATION");
                mfx_res = MFX_ERR_NONE;
            }
            if (MFX_ERR_NONE == mfx_res)
            {
                m_nSurfacesNumMin = MFX_OMX_MAX(request.NumFrameSuggested,
                                                MFX_OMX_MAX(request.NumFrameMin, 1));
                m_nSurfacesNum = MFX_OMX_MAX(m_nSurfacesNumMin, 4);

                if (m_bOnFlySurfacesAllocation && m_bANWBufferInMetaData)
                {
                    m_nSurfacesNumMin++;
                    m_nSurfacesNum++;
                }
            }
            MFX_OMX_AUTO_TRACE_I32(m_nSurfacesNumMin);
            MFX_OMX_AUTO_TRACE_I32(m_nSurfacesNum);
        }
    }
    if (MFX_INIT_DECODER == m_InitState)
    {
        MFX_OMX_AUTO_TRACE("MFX_INIT_DECODER");
        mfxU32 i = 0;
        mfxFrameAllocator* pAllocator = NULL;
        MfxOmxVaapiFrameAllocator* pvaAllocator = NULL;

        mfxFrameInfo allocFrameInfo = m_MfxVideoParams.mfx.FrameInfo;
        if (m_nMaxFrameWidth > allocFrameInfo.Width) allocFrameInfo.Width = m_nMaxFrameWidth;
        if (m_nMaxFrameHeight > allocFrameInfo.Height) allocFrameInfo.Height = m_nMaxFrameHeight;

        MFX_OMX_AUTO_TRACE_I32(m_bUseSystemMemory);
        if (m_pDevice && !m_bUseSystemMemory)
        {
            if ((MFX_ERR_NONE == mfx_res) && !m_pBufferHeaders) mfx_res = MFX_ERR_NULL_PTR;
            if (MFX_ERR_NONE == mfx_res)
            {
                pAllocator = m_pDevice->GetFrameAllocator();
                if (pAllocator) pvaAllocator = (MfxOmxVaapiFrameAllocator*)pAllocator;
                else mfx_res = MFX_ERR_UNKNOWN;
            }
            if (MFX_ERR_NONE == mfx_res)
                m_pSurfaces->SetFrameAllocator(pAllocator);

            for (i = 0; (MFX_ERR_NONE == mfx_res) && (i < m_pOutPortDef->nBufferCountActual); ++i)
            {
                if (MFX_ERR_NONE == mfx_res)
                {
                    MfxOmxBufferInfo* pAddBufInfo = NULL;
                    pAddBufInfo = (MfxOmxBufferInfo*)m_pBufferHeaders[i]->pOutputPortPrivate;

                    mfx_res = pvaAllocator->LoadSurface(m_bOnFlySurfacesAllocation ? NULL : (buffer_handle_t)m_pBufferHeaders[i]->pBuffer,
                                                        true, m_MfxVideoParams.mfx.FrameInfo,
                                                        &pAddBufInfo->sSurface.Data.MemId);
                }
            }
            // Frame allocator initialization (if needed)
            if (MFX_ERR_NONE == mfx_res)
            {
                mfxFrameAllocRequest request;
                MFX_OMX_ZERO_MEMORY(request);
                MFX_OMX_ZERO_MEMORY(m_AllocResponse);
                request.Info = allocFrameInfo;
                request.Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE |
                               MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET |
                               MFX_OMX_MEMTYPE_GRALLOC;
                request.NumFrameMin = request.NumFrameSuggested = m_pOutPortDef->nBufferCountActual;

                mfx_res = pAllocator->Alloc(pAllocator->pthis, &request, &m_AllocResponse);
            }
            if ((MFX_ERR_NONE == mfx_res) && (m_pOutPortDef->nBufferCountActual != m_AllocResponse.NumFrameActual))
            {
                mfx_res = MFX_ERR_MEMORY_ALLOC;
            }
        }
        MFX_OMX_AUTO_TRACE_MSG("PrepareSurfaces");
        for (i = 0; (MFX_ERR_NONE == mfx_res) && (i < m_pOutPortDef->nBufferCountActual); ++i)
        {
            mfx_res = m_pSurfaces->PrepareSurface(m_pBufferHeaders[i], &(allocFrameInfo),
                                                  (!m_bUseSystemMemory) ? m_AllocResponse.mids[i]: NULL);
        }

        if (MFX_ERR_NONE == mfx_res)
        {
            if (!m_bOnFlySurfacesAllocation)
            {
                mfx_res = m_pDEC->Init(&m_MfxVideoParams);
            }
            else
            {
                MFX_OMX_ZERO_MEMORY(m_adaptivePlayback);
                m_adaptivePlayback.BufferId = MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK;
                m_adaptivePlayback.BufferSz = sizeof(mfxExtBuffer);

                m_extBuffers.push_back(reinterpret_cast<mfxExtBuffer*>(&m_adaptivePlayback));
                m_MfxVideoParams.NumExtParam = m_extBuffers.size();
                m_MfxVideoParams.ExtParam = &m_extBuffers.front();

                mfx_res = m_pDEC->Init(&m_MfxVideoParams);

                m_extBuffers.pop_back();
                m_MfxVideoParams.NumExtParam--;
            }
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Decoder initialized with sts %d", mfx_res);

            if (MFX_WRN_PARTIAL_ACCELERATION == mfx_res)
            {
                MFX_OMX_LOG_INFO("MFX_WRN_PARTIAL_ACCELERATION");
                mfx_res = MFX_ERR_NONE;
            }
            else if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
            {
                MFX_OMX_AUTO_TRACE_MSG("MFX_WRN_INCOMPATIBLE_VIDEO_PARAM was received");
                mfx_res = MFX_ERR_NONE;
            }

            if (MFX_ERR_NONE != mfx_res) MFX_OMX_LOG_ERROR("Init failed - %s", mfx_omx_code_to_string(mfx_res));
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = m_pDEC->GetVideoParam(&m_MfxVideoParams);
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            m_InitState = MFX_INIT_COMPLETED;
        }
        MFX_OMX_AT__mfxVideoParam_dec(m_MfxVideoParams);
    }
    if (MFX_ERR_MORE_DATA != mfx_res)
    {
        if (MFX_ERR_NONE == mfx_res)
        {
            if (MFX_INIT_QUERY_IO_SURF == m_InitState)
            {
                if (m_pOutPortDef->format.video.nFrameWidth  != m_MfxVideoParams.mfx.FrameInfo.Width  ||
                    m_pOutPortDef->format.video.nFrameHeight != m_MfxVideoParams.mfx.FrameInfo.Height ||
                    m_nSurfacesNumMin > m_pOutPortDef->nBufferCountMin ||
                    m_nSurfacesNum > m_pOutPortDef->nBufferCountActual ||
                    m_bLegacyAdaptivePlayback ||
                    m_bEnableVP)
                {
                    m_bChangeOutputPortSettings = true;
                    MfxVideoParams_2_PortsParams();
                    MFX_OMX_AUTO_TRACE_MSG("Requesting change of output port settings");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Requesting change of output port settings");
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, OMX_IndexParamPortDefinition, NULL);
                }
                else if (m_colorAspects.IsColorAspectsChanged())
                {
                    MFX_OMX_AUTO_TRACE_MSG("Color aspects parsed from bitsream is defferent than passed from framework");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Color aspects parsed from bitsream is defferent than passed from framework");

                    m_colorAspects.SignalChangedColorAspectsIsSent();
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, MfxOmx_IndexGoogleDescribeColorAspects, NULL);
                }
            }
        }
        else
        {
            CloseCodec();
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxVdecComponent::IsResolutionChanged(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_pBitstream = m_pOmxBitstream->GetFrameConstructor()->GetMfxBitstream();
    MFX_OMX_AUTO_TRACE_P(m_pBitstream);
    // saving parameters
    mfxVideoParam newVideoParams = m_MfxVideoParams;
    // decoding header
    if (m_pDEC)
    {
        mfx_res = m_pDEC->DecodeHeader(m_pBitstream, &newVideoParams);
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("m_pDEC pointer is null");
        return false;
    }
    MFX_OMX_AUTO_TRACE_I32(newVideoParams.mfx.FrameInfo.Width);
    MFX_OMX_AUTO_TRACE_I32(newVideoParams.mfx.FrameInfo.Height);
    MFX_OMX_AUTO_TRACE_I32(m_MfxVideoParams.mfx.FrameInfo.Width);
    MFX_OMX_AUTO_TRACE_I32(m_MfxVideoParams.mfx.FrameInfo.Height);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    if (MFX_ERR_NONE != mfx_res)
    {
         MFX_OMX_AUTO_TRACE_MSG("DecodeHeader failed");
         return false; // assuming that if DecodeHeader fails, no resolution change
    }
    else
    {
        if (newVideoParams.mfx.FrameInfo.Width  == m_MfxVideoParams.mfx.FrameInfo.Width &&
            newVideoParams.mfx.FrameInfo.Height == m_MfxVideoParams.mfx.FrameInfo.Height)
        {
            MFX_OMX_AUTO_TRACE_MSG("No resolution change");
            return false;
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("Resolution is changed");
            return true;
        }
    }
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::ReinitCodec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_bEosHandlingStarted = false;
    m_bReinit = false;

    m_pBitstream = m_pOmxBitstream->GetFrameConstructor()->GetMfxBitstream();
    MFX_OMX_AUTO_TRACE_P(m_pBitstream);

    mfxVideoParam newVideoParams = m_MfxVideoParams;

    if (m_pDEC) mfx_res = m_pDEC->DecodeHeader(m_pBitstream, &newVideoParams);
    else return MFX_ERR_NULL_PTR;

    MFX_OMX_LOG("DecodeHeader %d", mfx_res);
    MFX_OMX_AUTO_TRACE_I32(m_nMaxFrameWidth);
    MFX_OMX_AUTO_TRACE_I32(m_nMaxFrameHeight);
    MFX_OMX_AUTO_TRACE_I32(newVideoParams.mfx.FrameInfo.Width);
    MFX_OMX_AUTO_TRACE_I32(newVideoParams.mfx.FrameInfo.Height);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("Failed to DecodeHeader");
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        if (newVideoParams.mfx.FrameInfo.Width > 8192 || newVideoParams.mfx.FrameInfo.Height > 8192)
        {
            mfx_res = MFX_ERR_UNSUPPORTED;
        }
    }

    bool bNeedChangePortSetting = false;
    if (MFX_ERR_NONE == mfx_res)
    {
        // check SPS for detect interlaced content
        bool bInterlaced;
        if (m_pOmxBitstream->GetFrameConstructor()->IsSetInterlaceFlag(&bInterlaced))
            m_bInterlaced = bInterlaced;
        else
            MFX_OMX_AUTO_TRACE_MSG("WARNING: m_bInterlaced was not set");
        MFX_OMX_AUTO_TRACE_I32(m_bInterlaced);

        m_colorAspects.UpdateBitsreamColorAspects(m_signalInfo);

        if (newVideoParams.mfx.FrameInfo.Width > m_nMaxFrameWidth ||
            newVideoParams.mfx.FrameInfo.Height > m_nMaxFrameHeight)
        {
            MFX_OMX_AUTO_TRACE_MSG("New resolution exceeds old");
            bNeedChangePortSetting = true;

            m_nMaxFrameWidth = newVideoParams.mfx.FrameInfo.Width;
            m_nMaxFrameHeight = newVideoParams.mfx.FrameInfo.Height;
        }
        else if (m_bOnFlySurfacesAllocation)
        {
            MFX_OMX_AUTO_TRACE_MSG("New resolution doesn't exceed old, but onFlySurfacesAllocation is turned on");
            bNeedChangePortSetting = true;

            m_nMaxFrameWidth = newVideoParams.mfx.FrameInfo.Width;
            m_nMaxFrameHeight = newVideoParams.mfx.FrameInfo.Height;
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("New resolution doesn't exceed old. Trying to reset decoder");
            mfx_res = m_pDEC->Reset(&newVideoParams);
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_AUTO_TRACE_MSG("Reset decoder failed. Calling reinit");
                mfx_res = MFX_ERR_NONE;

                mfxU32 nSurfacesRequired;

                mfxFrameAllocRequest request;
                MFX_OMX_ZERO_MEMORY(request);
                mfx_res = m_pDEC->QueryIOSurf(&newVideoParams, &request);
                if (MFX_WRN_PARTIAL_ACCELERATION == mfx_res)
                {
                    MFX_OMX_LOG_INFO("MFX_WRN_PARTIAL_ACCELERATION");
                    mfx_res = MFX_ERR_NONE;
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    nSurfacesRequired = MFX_OMX_MAX(request.NumFrameSuggested, MFX_OMX_MAX(request.NumFrameMin, 1));
                    MFX_OMX_AUTO_TRACE_I32(nSurfacesRequired);
                    MFX_OMX_AUTO_TRACE_I32(m_pOutPortDef->nBufferCountMin);
                    if (nSurfacesRequired <= m_pOutPortDef->nBufferCountMin)
                    {
                        mfx_res = m_pDEC->Close();
                        if (MFX_ERR_NONE == mfx_res)
                        {
                            mfx_res = m_pDEC->Init(&newVideoParams);
                            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Decoder initialized with sts %d", mfx_res);

                            if (MFX_WRN_PARTIAL_ACCELERATION == mfx_res)
                            {
                                MFX_OMX_LOG_INFO("MFX_WRN_PARTIAL_ACCELERATION");
                                mfx_res = MFX_ERR_NONE;
                            }
                            else if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
                            {
                                MFX_OMX_AUTO_TRACE_MSG("MFX_WRN_INCOMPATIBLE_VIDEO_PARAM was received");
                                mfx_res = MFX_ERR_NONE;
                            }
                            if (MFX_ERR_NONE != mfx_res) MFX_OMX_LOG_ERROR("Init failed - %s", mfx_omx_code_to_string(mfx_res));
                        }
                    }
                    else
                        bNeedChangePortSetting = true;
                }
            }
        }
    }

    bool bIsEventWasSent = false;
    if (MFX_ERR_NONE == mfx_res)
    {
        if (bNeedChangePortSetting)
        {
            m_bChangeOutputPortSettings = true;

            mfx_res = m_pDEC->Close();
            if (MFX_ERR_NONE == mfx_res)
            {
                if (m_pDevice && !m_bUseSystemMemory)
                {
                    mfxFrameAllocator* pAllocator = m_pDevice->GetFrameAllocator();

                    if (pAllocator) pAllocator->Free(pAllocator->pthis, &m_AllocResponse);
                    MFX_OMX_ZERO_MEMORY(m_AllocResponse);
                }

                MFX_OMX_AUTO_TRACE_MSG("Requesting change of output port settings");

                mfxFrameAllocRequest request;
                MFX_OMX_ZERO_MEMORY(request);

                mfx_res = m_pDEC->QueryIOSurf(&newVideoParams, &request);
                if (MFX_WRN_PARTIAL_ACCELERATION == mfx_res)
                {
                    MFX_OMX_LOG_INFO("MFX_WRN_PARTIAL_ACCELERATION");
                    mfx_res = MFX_ERR_NONE;
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    m_nSurfacesNumMin = MFX_OMX_MAX(request.NumFrameSuggested,
                                                    MFX_OMX_MAX(request.NumFrameMin, 1));
                    m_nSurfacesNum = MFX_OMX_MAX(m_nSurfacesNumMin, 4);

                    if (m_bOnFlySurfacesAllocation && m_bANWBufferInMetaData)
                    {
                        m_nSurfacesNumMin++;
                        m_nSurfacesNum++;
                    }
                }
                MFX_OMX_AUTO_TRACE_I32(m_nSurfacesNumMin);
                MFX_OMX_AUTO_TRACE_I32(m_nSurfacesNum);
                {
                    MfxOmxAutoLock lock(m_decoderMutex);
                    m_MfxVideoParams = newVideoParams;
                    MfxVideoParams_2_PortsParams();
                }

                if (MFX_ERR_NONE == mfx_res)
                {
                    MFX_OMX_AUTO_TRACE_MSG("Sending OMX_EventPortSettingsChanged to OMAX client: OMX_IndexParamPortDefinition");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Requesting change of output port definition");
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, OMX_IndexParamPortDefinition, NULL);
                    bIsEventWasSent = true;
                }
            }
        }
        else
        {
            {
                MfxOmxAutoLock lock(m_decoderMutex);
                m_MfxVideoParams = newVideoParams;
                MfxVideoParams_2_PortsParams();
                m_Crops = m_MfxVideoParams.mfx.FrameInfo;
            }
            MFX_OMX_AUTO_TRACE_MSG("Sending OMX_EventPortSettingsChanged to OMAX client: OMX_IndexConfigCommonOutputCrop");
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Requesting change of output port crop");
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, OMX_IndexConfigCommonOutputCrop, NULL);
            bIsEventWasSent = true;
        }
    }

    if (!bIsEventWasSent && m_colorAspects.IsColorAspectsChanged())
    {
        MFX_OMX_AUTO_TRACE_MSG("Color aspects parsed from bitsream is defferent than passed from framework");
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Color aspects parsed from bitsream is defferent than passed from framework");

        m_colorAspects.SignalChangedColorAspectsIsSent();
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, MfxOmx_IndexGoogleDescribeColorAspects, NULL);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::GetCurrentHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (NULL == pSPS || NULL == pPPS) mfx_res = MFX_ERR_NULL_PTR;

    if (MFX_ERR_NONE == mfx_res)
    {
        mfxExtCodingOptionSPSPPS spspps;
        MFX_OMX_ZERO_MEMORY(spspps);

        mfxVideoParam par;
        MFX_OMX_ZERO_MEMORY(par);

        spspps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
        spspps.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
        spspps.SPSBuffer = pSPS->Data;
        spspps.SPSBufSize = pSPS->MaxLength;
        spspps.PPSBuffer = pPPS->Data;
        spspps.PPSBufSize = pPPS->MaxLength;

        mfxExtBuffer* pExtBuf = &spspps.Header;

        par.NumExtParam = 1;
        par.ExtParam = &pExtBuf;

        if (m_pDEC) mfx_res = m_pDEC->GetVideoParam(&par);

        if (MFX_ERR_NONE == mfx_res)
        {
            pSPS->DataLength = spspps.SPSBufSize;
            pPPS->DataLength = spspps.PPSBufSize;
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::ResetInput(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = m_pOmxBitstream->Reset();

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::ResetOutput(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if ((MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId) || (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId))
    {
        mfxBitstream sps;
        MFX_OMX_ZERO_MEMORY(sps);
        mfxBitstream pps;
        MFX_OMX_ZERO_MEMORY(pps);
        mfxU8 buf[200] = {0};
        sps.Data = buf;
        sps.MaxLength = MFX_OMX_GET_ARRAY_SIZE(buf)/2;
        pps.Data = buf + sps.MaxLength;
        pps.MaxLength = MFX_OMX_GET_ARRAY_SIZE(buf)/2;
        if (MFX_ERR_NONE == GetCurrentHeaders(&sps, &pps))
        {
            m_pOmxBitstream->GetFrameConstructor()->SaveHeaders(&sps, &pps, true);
        }
    }

    if (m_pDEC)
    {
        mfx_res = m_pDEC->Reset(&m_MfxVideoParams);
    }

    if (MFX_ERR_NOT_INITIALIZED == mfx_res) mfx_res = MFX_ERR_NONE;

    m_pSurfaces->Reset();

    m_bEosHandlingStarted = false;
    m_bEosHandlingFinished = false;
    m_bFlush = false;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::ResetCodec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE, mfx_sts = MFX_ERR_NONE;

    mfx_sts = ResetInput();
    if (MFX_ERR_NONE == mfx_res) mfx_res = mfx_sts;
    mfx_sts = ResetOutput();
    if (MFX_ERR_NONE == mfx_res) mfx_res = mfx_sts;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::CloseCodec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (m_pDEC) m_pDEC->Close();
    if (m_pOmxBitstream)
    {
        m_pOmxBitstream->Reset();
        m_pOmxBitstream->GetFrameConstructor()->Close();
    }
    // swap pBufInfo for each buffer before closing surface pool because after that we lose swap information and get error in freeBuffer
    if (m_bANWBufferInMetaData)
    {
        for (mfxU32 i = 0; i < m_nBufferHeadersSize; ++i)
        {
            if (m_pBufferHeaders[i])
            {
                m_pSurfaces->SwapBufferInfo(m_pBufferHeaders[i]);
            }
        }
    }
    m_pSurfaces->Close();

    if (m_pDevice && !m_bUseSystemMemory)
    {
        mfxFrameAllocator* pAllocator = m_pDevice->GetFrameAllocator();
        if (pAllocator) pAllocator->Free(pAllocator->pthis, &m_AllocResponse);
        MFX_OMX_ZERO_MEMORY(m_AllocResponse);
    }

    do
    {
        MFX_OMX_FREE(m_pFreeSyncPoint);
    }
    while (m_SyncPoints.Get(&m_pFreeSyncPoint, g_NilSyncPoint));

    MFX_OMX_FREE(m_pBufferHeaders);
    m_nBufferHeadersSize = 0;

    // return parameters to initial state
    m_InitState = MFX_INIT_DECODE_HEADER;
    m_nSurfacesNum = 1;
    m_nSurfacesNumMin = 1;
    m_bChangeOutputPortSettings = false;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxVdecComponent::CanDecode(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (m_bChangeOutputPortSettings) return false;
    if (!m_pSurfaces->GetBuffer())
    {
        MFX_OMX_AUTO_TRACE_MSG("Free surface not found");
        return false;
    }
    if (!m_pFreeSyncPoint && !m_SyncPoints.Get(&m_pFreeSyncPoint, g_NilSyncPoint))
    {
        MFX_OMX_AUTO_TRACE_MSG("Free sync point not found!"); // should never occur !!!
        return false;
    }
    return true;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVdecComponent::DecodeFrame(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxFrameSurface1 *pWorkSurface = NULL, *pOutSurface = NULL;
    mfxSyncPoint* pSyncPoint = NULL;

    MFX_OMX_AUTO_TRACE_P(m_pBitstream);

    if (m_MfxVideoParams.mfx.FrameInfo.PicStruct != MFX_PICSTRUCT_PROGRESSIVE)
    {
        if (m_pBitstream) m_pBitstream->DataFlag &= ~MFX_BITSTREAM_COMPLETE_FRAME;
    }

    //VP9 and MPEG2 decoders able to flush without work surface
    bool bIsFlushingWithoutWorkSurf = ((m_MfxVideoParams.mfx.CodecId == MFX_CODEC_VP9 || m_MfxVideoParams.mfx.CodecId == MFX_CODEC_MPEG2) && NULL == m_pBitstream);
    //application can't submit without SyncOperation() more than AsyncDepth frames
    bool bCanSubmit = (m_MfxVideoParams.AsyncDepth == 0 || m_MfxVideoParams.AsyncDepth > m_pSurfaces->GetNumSubmittedSurfaces());

    while (((MFX_ERR_NONE == mfx_res) || (MFX_ERR_MORE_SURFACE == mfx_res)) &&
           (m_InitState > MFX_INIT_DECODE_HEADER) &&
           CheckBitstream(m_pBitstream) && bCanSubmit)
    {
        MFX_OMX_AUTO_TRACE("Decoding loop");

        if (m_pBitstream && m_pBitstream->DataLength == 0)
        {
            mfx_res = MFX_ERR_MORE_DATA;
            break;
        }

        pOutSurface = NULL;
        pWorkSurface = m_pSurfaces->GetBuffer();
        MFX_OMX_AUTO_TRACE_P(pWorkSurface);
        if (NULL == pWorkSurface && !bIsFlushingWithoutWorkSurf)
        {
            MFX_OMX_AUTO_TRACE_MSG("Free surface not found");
            mfx_res = MFX_ERR_MORE_SURFACE;
            break;
        }
        if (!m_pFreeSyncPoint && !m_SyncPoints.Get(&m_pFreeSyncPoint, g_NilSyncPoint))
        {
            MFX_OMX_AUTO_TRACE_MSG("Free sync point not found!");
            mfx_res = MFX_ERR_NOT_FOUND;
            break;
        }
        else pSyncPoint = m_pFreeSyncPoint;

        if (NULL != pWorkSurface) pWorkSurface->Data.FrameOrder = m_nLockedSurfacesNum;
        // Decoding bitstream. If function called on EOS then drainnig begins and we must
        // wait while decoder can accept input data
        do
        {
            //this actions is required to prevent decoder overflow
            //in case of overflow and NULL work surface decoder returns unexpected not NULL surface which leads to incorrect results
            //m_nSurfaceNum - 1 because decoder have 1 more surface than we can submit to it without freeing buffers
            //TODO: remove when this decoder behavior stop to occur
            if (bIsFlushingWithoutWorkSurf && (pWorkSurface == NULL) && 
                (m_pSurfaces->GetNumSubmittedSurfaces() >= m_nSurfacesNum - 1 )) {
                using namespace std::chrono_literals;
                //using pause equal to 1ms which equal to theoretical minimum of frame decoding
                std::this_thread::sleep_for(1ms);
                continue;
            }
            if (m_pBitstream) MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "DecodeFrameAsync+ DataLength %d, DataOffset %d", m_pBitstream->DataLength, m_pBitstream->DataOffset);
            else MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "DecodeFrameAsync(NULL)+");

            mfx_res = m_pDEC->DecodeFrameAsync(m_pBitstream, pWorkSurface, &pOutSurface, pSyncPoint);

            if (m_pBitstream) MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "DecodeFrameAsync- sts %d, output surface %p, DataLength %d, DataOffset %d",
                                    mfx_res, pOutSurface, m_pBitstream->DataLength, m_pBitstream->DataOffset);
            else MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "DecodeFrameAsync(NULL)- sts %d, output surface %p", mfx_res, pOutSurface);

            if(pOutSurface)
            {
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel,"pOutSurface, CropW: %u, CropH: %u, Width: %u, Height: %u, TimeStamp: %llu",
                        pOutSurface->Info.CropW, pOutSurface->Info.CropH, pOutSurface->Info.Width, pOutSurface->Info.Height, pOutSurface->Data.TimeStamp);
            }
            if (MFX_WRN_DEVICE_BUSY == mfx_res)
            {
                mfxStatus mfx_sts = MFX_ERR_NONE;
                MFX_OMX_AUTO_TRACE("mfx(DecodeFrameAsync)::MFX_WRN_DEVICE_BUSY");
                mfxSyncPoint* pDevBusySyncPoint = m_pSurfaces->GetSyncPoint();
                if (pDevBusySyncPoint)
                {
                    mfx_sts = m_Session.SyncOperation(*pDevBusySyncPoint, MFX_OMX_INFINITE);
                }
                else
                {
                    m_pDevBusyEvent->TimedWait(1);
                    m_pDevBusyEvent->Reset();
                }
                if (MFX_ERR_NONE != mfx_sts)
                {
                    MFX_OMX_LOG_ERROR("SyncOperation failed - %s", mfx_omx_code_to_string(mfx_sts));
                    mfx_res = mfx_sts;
                }
            }
        } while (MFX_WRN_DEVICE_BUSY == mfx_res);
        // valid cases for the status are:
        // MFX_ERR_NONE - data processed, output will be generated
        // MFX_ERR_MORE_DATA - data buffered, output will not be generated
        // MFX_ERR_MORE_SURFACE - need one more free surface
        // MFX_WRN_VIDEO_PARAM_CHANGED - some params changed, but decoding can be continued
        // MFX_ERR_INCOMPATIBLE_VIDEO_PARAM - need to reinitialize decoder with new params
        // status correction
        if (MFX_WRN_VIDEO_PARAM_CHANGED == mfx_res) mfx_res = MFX_ERR_MORE_SURFACE;

        if ((MFX_ERR_NONE == mfx_res) || (MFX_ERR_MORE_DATA == mfx_res) || (MFX_ERR_MORE_SURFACE == mfx_res))
        {
            if (MFX_ERR_MORE_DATA == mfx_res && m_pOmxBitstream->GetFrameConstructor()->WasEosReached() &&
                m_pBitstream && m_pBitstream->DataLength > 0)
            {
                // skip useless tail of bitstream which can't be decoded
                m_pBitstream->DataLength = 0;
            }

            if (NULL != pWorkSurface && pWorkSurface->Data.Locked) ++m_nLockedSurfacesNum;

            if (NULL != pOutSurface)
            {
                MFX_OMX_AUTO_TRACE_I32(pOutSurface->Data.Locked);
                MFX_OMX_AUTO_TRACE_I64(pOutSurface->Data.TimeStamp);

#ifdef HEVC10HDR_SUPPORT
                UpdateHdrStaticInfo();
#endif

                if (pOutSurface->Info.CropW != m_MfxVideoParams.mfx.FrameInfo.CropW ||
                    pOutSurface->Info.CropH != m_MfxVideoParams.mfx.FrameInfo.CropH)
                {
                    MfxOmxAutoLock lock(m_decoderMutex);
                    m_MfxVideoParams.mfx.FrameInfo.CropW = pOutSurface->Info.CropW;
                    m_MfxVideoParams.mfx.FrameInfo.CropH = pOutSurface->Info.CropH;
                    MfxVideoParams_2_PortsParams();

                    MFX_OMX_AUTO_TRACE_MSG("Sending OMX_EventPortSettingsChanged to OMAX client: OMX_IndexConfigCommonOutputCrop");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Requesting change of output port crop");
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, OMX_IndexConfigCommonOutputCrop, NULL);
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    // searching for buffer which corresponds to the surface
                    mfx_res = m_pSurfaces->QueueBufferForSending(pOutSurface, pSyncPoint);
                    if (MFX_ERR_NONE != mfx_res)
                    {
                        /* NOTE: that's workaround for the VC1 skipped frames support.
                         * VC1 can return the same surface few times. We do not support this
                         * at the moment on plug-in level and just drop the frame. Actually
                         * skipped frame is not complete in Media SDK: normally library should
                         * always return unique surface.
                         */
                        mfx_res = MFX_ERR_NONE;
                    }
                    else
                    {
                        m_nCountDecodedFrames++;
                        m_pFreeSyncPoint = NULL;

                        if (MFX_ERR_NONE == mfx_res)
                        {
                            m_pAllSyncOpFinished->Reset();
                            m_pAsyncSemaphore->Post();
                        }
                    }
                }
                if (m_MfxVideoParams.AsyncDepth != 0 && m_MfxVideoParams.AsyncDepth <= m_pSurfaces->GetNumSubmittedSurfaces())
                    bCanSubmit = false;
            }
        }
        else if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("MFX_ERR_INCOMPATIBLE_VIDEO_PARAM: resolution was changed");
            m_bReinit = true;
            m_bEosHandlingStarted = true; // we need to obtain buffered frames
            if (!m_SyncPoints.Add(&pSyncPoint)) mfx_res = MFX_ERR_UNKNOWN;
            m_pFreeSyncPoint = NULL;
            break;
        }
        else
        {
            MFX_OMX_LOG_ERROR("DecodeFrameAsync failed - %s", mfx_omx_code_to_string(mfx_res));
        }
        MFX_OMX_AUTO_TRACE_I32(mfx_res);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVdecComponent::AsyncThread(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxSyncPoint* pSyncPoint = NULL;
    while (1)
    {
        m_pAsyncSemaphore->Wait();

        if (m_bDestroy) break;

        MFX_OMX_AUTO_TRACE("Async Thread Loop iteration");

        if (m_pSurfaces)
        {
            pSyncPoint = m_pSurfaces->GetSyncPoint();
            if (pSyncPoint)
            {
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation+");
                    MFX_OMX_AUTO_TRACE("SyncOperation");
                    mfx_res = m_Session.SyncOperation(*pSyncPoint, MFX_OMX_INFINITE);
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation-");
                    m_pDevBusyEvent->Signal();
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    if (!m_SyncPoints.Add(&pSyncPoint)) mfx_res = MFX_ERR_UNKNOWN;
                }
                else
                {
                    MFX_OMX_LOG_ERROR("SyncOperation failed - %s", mfx_omx_code_to_string(mfx_res));
                }

                if (MFX_ERR_NONE == mfx_res)
                {
                    m_pSurfaces->DisplaySurface(m_bErrorReportingEnabled);
                }
                if (MFX_ERR_NONE != mfx_res) // Error processing
                {
                    m_Error = mfx_res;
                    MFX_OMX_LOG_ERROR("Sending ErrorEvent to OMAX client because of error in component");
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "AsyncThread : m_Error = %d", m_Error);
                    MFX_OMX_AUTO_TRACE_I32(m_Error);
                    m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(m_Error), 0 , NULL);
                }
            }
            else
            {
                MFX_OMX_AUTO_TRACE("Sending EOS frame and event");
                m_pSurfaces->DisplaySurface(m_bErrorReportingEnabled);
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventBufferFlag, MFX_OMX_OUTPUT_PORT_INDEX, OMX_BUFFERFLAG_EOS, NULL);
            }
        }

        if ((NULL == m_pSurfaces) || (m_pSurfaces && (NULL == m_pSurfaces->GetSyncPoint())) || (MFX_ERR_NONE != m_Error))
        {
            m_pAllSyncOpFinished->Signal();
        }
    }
}

/*------------------------------------------------------------------------------*/

#ifdef HEVC10HDR_SUPPORT
void MfxOmxVdecComponent::UpdateHdrStaticInfo()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxPayload* pHdrSeiPayload = m_pOmxBitstream->GetFrameConstructor()->GetSei(MfxOmxHEVCFrameConstructor::SEI_MASTERING_DISPLAY_COLOUR_VOLUME);

    const mfxU32 SEI_MASTERING_DISPLAY_COLOUR_VOLUME_SIZE = 24*8; // required size of data in bits

    if (nullptr != pHdrSeiPayload && pHdrSeiPayload->NumBit >= SEI_MASTERING_DISPLAY_COLOUR_VOLUME_SIZE && nullptr != pHdrSeiPayload->Data)
    {
        MFX_OMX_AUTO_TRACE_MSG("Set HDR static info: SEI_MASTERING_DISPLAY_COLOUR_VOLUME");

        m_bIsSetHDRSEI = true;
        m_SeiHDRStaticInfo.mID = android::HDRStaticInfo::kType1;
        m_SeiHDRStaticInfo.sType1.mR.x = pHdrSeiPayload->Data[1] | (pHdrSeiPayload->Data[0] << 8);
        m_SeiHDRStaticInfo.sType1.mR.y = pHdrSeiPayload->Data[3] | (pHdrSeiPayload->Data[2] << 8);
        m_SeiHDRStaticInfo.sType1.mG.x = pHdrSeiPayload->Data[5] | (pHdrSeiPayload->Data[4] << 8);
        m_SeiHDRStaticInfo.sType1.mG.y = pHdrSeiPayload->Data[7] | (pHdrSeiPayload->Data[6] << 8);
        m_SeiHDRStaticInfo.sType1.mB.x = pHdrSeiPayload->Data[9] | (pHdrSeiPayload->Data[8] << 8);
        m_SeiHDRStaticInfo.sType1.mB.y = pHdrSeiPayload->Data[11] | (pHdrSeiPayload->Data[10] << 8);
        m_SeiHDRStaticInfo.sType1.mW.x = pHdrSeiPayload->Data[13] | (pHdrSeiPayload->Data[12] << 8);
        m_SeiHDRStaticInfo.sType1.mW.y = pHdrSeiPayload->Data[15] | (pHdrSeiPayload->Data[14] << 8);

        mfxU32 mMaxDisplayLuminanceX10000 = pHdrSeiPayload->Data[19] | (pHdrSeiPayload->Data[18] << 8) | (pHdrSeiPayload->Data[17] << 16) | (pHdrSeiPayload->Data[16] << 24);
        m_SeiHDRStaticInfo.sType1.mMaxDisplayLuminance = (mfxU16)(mMaxDisplayLuminanceX10000 / 10000);

        mfxU32 mMinDisplayLuminanceX10000 = pHdrSeiPayload->Data[23] | (pHdrSeiPayload->Data[22] << 8) | (pHdrSeiPayload->Data[21] << 16) | (pHdrSeiPayload->Data[20] << 24);
        m_SeiHDRStaticInfo.sType1.mMinDisplayLuminance = (mfxU16)(mMinDisplayLuminanceX10000 / 10000);
    }
    pHdrSeiPayload = m_pOmxBitstream->GetFrameConstructor()->GetSei(MfxOmxHEVCFrameConstructor::SEI_CONTENT_LIGHT_LEVEL_INFO);

    const mfxU32 SEI_CONTENT_LIGHT_LEVEL_INFO_SIZE = 4*8; // required size of data in bits

    if (nullptr != pHdrSeiPayload && pHdrSeiPayload->NumBit >= SEI_CONTENT_LIGHT_LEVEL_INFO_SIZE && nullptr != pHdrSeiPayload->Data)
    {
        MFX_OMX_AUTO_TRACE_MSG("Set HDR static info: SEI_CONTENT_LIGHT_LEVEL_INFO");

        m_SeiHDRStaticInfo.sType1.mMaxContentLightLevel = pHdrSeiPayload->Data[1] | (pHdrSeiPayload->Data[0] << 8);
        m_SeiHDRStaticInfo.sType1.mMaxFrameAverageLightLevel = pHdrSeiPayload->Data[3] | (pHdrSeiPayload->Data[2] << 8);
    }
}
#endif

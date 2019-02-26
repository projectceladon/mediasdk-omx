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
#include "mfx_omx_venc_component.h"
#include "mfx_omx_vaapi_allocator.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_venc_component"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_ENCIN_FILE "/data/mfx/mfx_omx_encin.yuv"
#define MFX_OMX_ENCOUT_FILE "/data/mfx/mfx_omx_encout"

/*------------------------------------------------------------------------------*/

MfxOmxComponent* MfxOmxVencComponent::Create(
    OMX_HANDLETYPE self,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    MfxOmxVencComponent* pComponent = NULL;

    MFX_OMX_NEW(pComponent, MfxOmxVencComponent(omx_res, self, reg_data, flags));
    if (OMX_ErrorNone == omx_res)
    {
        if (pComponent) omx_res = pComponent->Init();
        else omx_res = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone != omx_res)
    {
        MFX_OMX_DELETE(pComponent);
    }
    MFX_OMX_AUTO_TRACE_P(pComponent);
    return pComponent;
}

/*------------------------------------------------------------------------------*/

MfxOmxVencComponent::MfxOmxVencComponent(OMX_ERRORTYPE &error,
                                         OMX_HANDLETYPE self,
                                         MfxOmxComponentRegData* reg_data,
                                         OMX_U32 flags):
    MfxOmxComponent(error, self, reg_data, flags),
    m_Implementation(MFX_OMX_IMPLEMENTATION),
    m_pENC(NULL),
    m_MfxVideoParams(m_OmxMfxVideoParams),
    m_MfxEncodeCtrl(m_OmxMfxEncodeCtrl),
    m_eOmxControlRate(OMX_Video_ControlRateConstant),
    m_bInitialized(false),
    m_bChangeOutputPortSettings(false),
    m_bCanNotProcess(false),
    m_bEosHandlingStarted(false),
    m_bEosHandlingFinished(false),
    m_bCodecDataSent(false),
    m_bVppDetermined(false),
    m_bSkipThisFrame(false),
    m_bEnableInternalSkip(false),
    m_lBufferFullness(0),
    m_lCurTargetBitrate(0),
    m_pDevice(NULL),
    m_inputVppType(CONVERT_NONE),
    m_pBitstreams(NULL),
    m_nSurfacesNum(0),
    m_pSurfaces(NULL),
    m_pFreeSyncPoint(NULL),
    m_nEncoderInputSurfacesCount(0),
    m_nEncoderOutputBitstreamsCount(0),
    m_blackFrame(NULL),
    m_lastTimeStamp(0xFFFFFFFFFFFFFFFF),
    m_priority(MFX_OMX_PRIORITY_PERFORMANCE),
    m_dbg_encin(NULL),
    m_dbg_encout(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_ZERO_MEMORY(m_NextConfig);
    m_NextConfig.mfxparams = &m_OmxMfxVideoParams;
}

/*------------------------------------------------------------------------------*/

MfxOmxVencComponent::~MfxOmxVencComponent(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    InternalThreadsWait();

    if (m_blackFrame) m_pDevice->GetGrallocAllocator()->Free(m_blackFrame);

    MFX_OMX_DELETE(m_pBitstreams);
    MFX_OMX_DELETE(m_pSurfaces);
    MFX_OMX_DELETE(m_pENC);
    m_VPP.Close();
    m_Session.Close();

    MFX_OMX_DELETE(m_pDevice);

    if (m_dbg_encin) fclose(m_dbg_encin);
    if (m_dbg_encout) fclose(m_dbg_encout);
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Encoded %d frames", m_nEncoderOutputBitstreamsCount);
    MFX_OMX_LOG_INFO("Destroyed %s", m_pRegData->m_name);
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::Init(void)
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

        // encoder creation
        if (MFX_ERR_NONE == sts)
        {
            MFX_OMX_NEW(m_pENC, MFXVideoENCODE(m_Session));
            if (!m_pENC) sts = MFX_ERR_MEMORY_ALLOC;
        }
        if (MFX_ERR_NONE != sts) error = OMX_ErrorUndefined;
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
        if (MFX_ERR_NONE != sts) error = OMX_ErrorUndefined;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pSurfaces, MfxOmxInputSurfacesPool(sts));
        if ((MFX_ERR_NONE == sts) && !m_pSurfaces)
        {
            sts = MFX_ERR_NULL_PTR;
        }
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
        if (MFX_ERR_NONE == sts)
        {
            if (MFX_OMX_COMPONENT_FLAGS_DUMP_INPUT & m_Flags)
            {
                m_dbg_encin = fopen(MFX_OMX_ENCIN_FILE, "w");
            }
            m_pSurfaces->SetFile(m_dbg_encin);
        }
#endif
        if (MFX_ERR_NONE == sts)
        {
            sts = m_pSurfaces->SetBuffersCallback(this);
        }
        if ((MFX_ERR_NONE == sts) && (MFX_IMPL_SOFTWARE != m_Implementation))
        {
            sts = m_pSurfaces->SetMfxDevice(m_pDevice);
        }
        if (MFX_ERR_NONE != sts) error = OMX_ErrorUndefined;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pBitstreams, MfxOmxBitstreamsPool(m_MfxVideoParams.mfx.CodecId, sts));
        if ((MFX_ERR_NONE == sts) && !m_pBitstreams)
        {
            sts = MFX_ERR_NULL_PTR;
        }
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
        if (MFX_ERR_NONE == sts)
        {
            if (MFX_OMX_COMPONENT_FLAGS_DUMP_OUTPUT & m_Flags)
            {
                m_dbg_encout = fopen(MFX_OMX_ENCOUT_FILE, "w");
            }
            m_pBitstreams->SetFile(m_dbg_encout);
        }
#endif
        if (MFX_ERR_NONE == sts)
        {
            sts = m_pBitstreams->SetBuffersCallback(this);
        }
        if (MFX_ERR_NONE != sts) error = OMX_ErrorUndefined;
    }
    if ((OMX_ErrorNone == error) && (MFX_IMPL_SOFTWARE != m_Implementation))
    {
        if ((MFX_HW_BXT == m_pDevice->GetPlatformType()) &&
            (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId))
            m_MfxVideoParams.mfx.LowPower = MFX_CODINGOPTION_ON;
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

void MfxOmxVencComponent::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    switch (m_pRegData->m_type)
    {
    case MfxOmx_h264ve:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
        break;
    case MfxOmx_h265ve:
        m_MfxVideoParams.mfx.CodecId = MFX_CODEC_HEVC;
        break;
    default:
        MFX_OMX_AUTO_TRACE_MSG("unhandled codec type: BUG in plug-ins registration");
        break;
    }
    mfx_omx_set_defaults_mfxVideoParam_enc(&m_MfxVideoParams);

    // default pattern
    m_MfxVideoParams.IOPattern = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
    m_OmxMfxVideoParams.ResetExtParams();
    PortsParams_2_MfxVideoParams();
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::Set_PortDefinition(
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::Set_PortDefinition(pPortDef);

    if (OMX_ErrorNone == omx_res)
    {
        if (MFX_OMX_INPUT_PORT_INDEX == pPortDef->nPortIndex)
        {
            m_nSurfacesNum = pPortDef->nBufferCountActual;

            // synchronizing output port parameters
            omx_res = InPortParams_2_OutPortParams();
            // synchronizing with mfxVideoParam
            if (OMX_ErrorNone == omx_res) omx_res = PortsParams_2_MfxVideoParams();
        }
        else if (MFX_OMX_OUTPUT_PORT_INDEX == pPortDef->nPortIndex)
        {
            // synchronizing with mfxVideoParam
            omx_res = PortsParams_2_MfxVideoParams();
        }
        else omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::Set_VideoPortFormat(
    OMX_VIDEO_PARAM_PORTFORMATTYPE* pVideoFormat)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::Set_VideoPortFormat(pVideoFormat);

    if (OMX_ErrorNone == omx_res)
    {
        if (MFX_OMX_INPUT_PORT_INDEX == pVideoFormat->nPortIndex)
        {
            // synchronizing output port parameters
            omx_res = InPortParams_2_OutPortParams();
            // synchronizing with mfxVideoParam
            if (OMX_ErrorNone == omx_res) {
                omx_res = PortsParams_2_MfxVideoParams();
            }
        }
        else if (MFX_OMX_OUTPUT_PORT_INDEX == pVideoFormat->nPortIndex)
        {
            // synchronizing with mfxVideoParam
            omx_res = PortsParams_2_MfxVideoParams();
        }
        else omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::InPortParams_2_OutPortParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    m_pOutPortDef->format.video.nFrameWidth = m_pInPortDef->format.video.nFrameWidth;
    m_pOutPortDef->format.video.nFrameHeight = m_pInPortDef->format.video.nFrameHeight;

    mfx_omx_adjust_port_definition(m_pOutPortDef, NULL,
                                   false, false);

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::PortsParams_2_MfxVideoParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxU16 align = 16;

    // data from input port

    m_MfxVideoParams.AsyncDepth = GetAsyncDepth();
    m_MfxVideoParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    m_MfxVideoParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    m_MfxVideoParams.mfx.FrameInfo.Width = (mfxU16) MFX_OMX_MEM_ALIGN(m_pInPortDef->format.video.nFrameWidth, align);
    m_MfxVideoParams.mfx.FrameInfo.Height = (mfxU16) MFX_OMX_MEM_ALIGN(m_pInPortDef->format.video.nFrameHeight, align);

    m_MfxVideoParams.mfx.FrameInfo.CropW = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    m_MfxVideoParams.mfx.FrameInfo.CropH = (mfxU16)m_pInPortDef->format.video.nFrameHeight;

    m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN = (mfxU32)(m_pInPortDef->format.video.xFramerate >> 16); // convert from Q16 format
    if (m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN != 0) m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD = 1;
    else m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD = 0;

    m_MfxVideoParams.mfx.RateControlMethod = MFX_RATECONTROL_CBR;
    m_MfxVideoParams.mfx.TargetKbps = (mfxU16)(m_pOutPortDef->format.video.nBitrate*0.001); // convert to Kbps

    int idx;
    if (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId)
    {
        idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION);
        if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            m_OmxMfxVideoParams.ext_buf[idx].opt.VuiNalHrdParameters = MFX_CODINGOPTION_OFF;
            m_OmxMfxVideoParams.ext_buf[idx].opt.PicTimingSEI = MFX_CODINGOPTION_OFF;
            m_OmxMfxVideoParams.ext_buf[idx].opt.AUDelimiter = MFX_CODINGOPTION_OFF;
        }

        idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
        if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            m_OmxMfxVideoParams.ext_buf[idx].opt2.RepeatPPS = MFX_CODINGOPTION_OFF;
        }

        static const mfxU32 maxFrameRate = 172;
        if (m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN > mfxU64(maxFrameRate) * m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD)
        {
            idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION3);
            if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
            {
                m_OmxMfxVideoParams.ext_buf[idx].opt3.TimingInfoPresent = MFX_CODINGOPTION_OFF;
            }
        }
    }
    else if (MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId)
    {
        static const mfxU32 maxFrameRate = 300;
        if (m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN > mfxU64(maxFrameRate) * m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD)
        {
            m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN = maxFrameRate;
        }

        idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION);
        if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            m_OmxMfxVideoParams.ext_buf[idx].opt.AUDelimiter = MFX_CODINGOPTION_OFF;
            m_OmxMfxVideoParams.ext_buf[idx].opt.VuiNalHrdParameters = MFX_CODINGOPTION_OFF;
            m_OmxMfxVideoParams.ext_buf[idx].opt.PicTimingSEI = MFX_CODINGOPTION_OFF;
        }
    }
    MFX_OMX_AT__mfxVideoParam_enc(m_MfxVideoParams);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::MfxVideoParams_2_PortsParams(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    m_pOutPortDef->format.video.nFrameWidth = m_MfxVideoParams.mfx.FrameInfo.Width;
    m_pOutPortDef->format.video.nFrameHeight = m_MfxVideoParams.mfx.FrameInfo.Height;

    m_pOutPortDef->bPopulated = OMX_FALSE;

    mfx_omx_adjust_port_definition(m_pOutPortDef, NULL,
                                   false, false);

    if (m_MfxVideoParams.mfx.BufferSizeInKB)
        m_pOutPortDef->nBufferSize = m_MfxVideoParams.mfx.BufferSizeInKB * 1000 * m_MfxVideoParams.mfx.BRCParamMultiplier;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::MfxVideoParams_Get_From_InPort(mfxFrameInfo* out_pMfxFrameInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    memset((void*)out_pMfxFrameInfo, 0, sizeof(mfxFrameInfo));

    out_pMfxFrameInfo->FourCC = MFX_FOURCC_NV12;
    out_pMfxFrameInfo->ChromaFormat = MFX_CHROMAFORMAT_YUV420;

    out_pMfxFrameInfo->Width = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    out_pMfxFrameInfo->Height = (mfxU16)m_pInPortDef->format.video.nFrameHeight;

    out_pMfxFrameInfo->CropW = (mfxU16)m_pInPortDef->format.video.nFrameWidth;
    out_pMfxFrameInfo->CropH = (mfxU16)m_pInPortDef->format.video.nFrameHeight;
    out_pMfxFrameInfo->CropX = 0;
    out_pMfxFrameInfo->CropY = 0;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

mfxU16 MfxOmxVencComponent::GetAsyncDepth(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxU16 asyncDepth = 1; // a default value
    mfxU32 frameRate = m_pInPortDef->format.video.xFramerate >> 16;

    if ((m_priority == MFX_OMX_PRIORITY_REALTIME) ||
        (OMX_Video_Intel_ControlRateVideoConferencingMode == m_eOmxControlRate) ||
        IsMiracastMode() ||  IsChromecastMode())
    {
        asyncDepth = 1;
    }
    else if (frameRate > 30) asyncDepth = 2; // 60 fps camera


    MFX_OMX_AUTO_TRACE_I32(asyncDepth);
    return asyncDepth;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxVencComponent::IsMiracastMode(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    bool bIsMiracast = false;

    mfxU32 frameRate = m_pInPortDef->format.video.xFramerate >> 16;
    // TODO: need to correct conditition later when VCM mode will be enabled,
    // i.e. need to compare m_MfxVideoParams.mfx.RateControlMethod != MFX_RATECONTROL_VCM
    // instead of similar condition for OMX port parameter
    if (( m_pBitstreams &&
         !m_pBitstreams->IsRemovingSPSPPSNeeded() &&
         (OMX_Video_ControlRateConstant == m_eOmxControlRate) &&
         (30 == frameRate))
    )
        bIsMiracast = true;

    MFX_OMX_AUTO_TRACE_I32(bIsMiracast);
    return bIsMiracast;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxVencComponent::IsChromecastMode(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    bool bIsChromecast = false;
    mfxU32 frameRate = m_pInPortDef->format.video.xFramerate >> 16;
    if ((m_pBitstreams && !m_pBitstreams->IsRemovingSPSPPSNeeded()) && (60 == frameRate) && (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId))
        bIsChromecast = true;

    MFX_OMX_AUTO_TRACE_I32(bIsChromecast);
    return bIsChromecast;
}

/*------------------------------------------------------------------------------*/

mfxU16 MfxOmxVencComponent::CalcNumSkippedFrames(mfxU64 currentTimeStamp)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxU32 framerate = m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN/m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD;
    mfxU64 expectedDelta = OMX_TICKS_PER_SECOND/framerate;
    mfxU64 delta = (currentTimeStamp > m_lastTimeStamp) ? (currentTimeStamp - m_lastTimeStamp) : expectedDelta;

    mfxU16 nFrameskipped = 0;
    if (delta > expectedDelta)
    {
        if (delta < 1.6*expectedDelta)
            nFrameskipped = 0;
        else if ((2.6*expectedDelta > delta) && (delta >= 1.6*expectedDelta))
            nFrameskipped = 1;
        else if ((3.6*expectedDelta > delta) && (delta >= 2.6*expectedDelta))
            nFrameskipped = 2;
        else
            nFrameskipped = 3;
    }

    MFX_OMX_AUTO_TRACE_I32(nFrameskipped);
    return nFrameskipped;
}

/*------------------------------------------------------------------------------*/

mfxU32 MfxOmxVencComponent::QueryMaxMbPerSec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxVideoParam params = m_MfxVideoParams;

    mfxExtEncoderCapability encCaps;
    MFX_OMX_ZERO_MEMORY(encCaps);
    encCaps.Header.BufferId = MFX_EXTBUFF_ENCODER_CAPABILITY;
    encCaps.Header.BufferSz = sizeof(mfxExtEncoderCapability);

    mfxExtBuffer* pExtBuf = &encCaps.Header;
    params.NumExtParam = 1;
    params.ExtParam = &pExtBuf;

    m_pENC->Query(&params, &params);

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "MBPerSec %d", encCaps.MBPerSec);
    MFX_OMX_AUTO_TRACE_I32(encCaps.MBPerSec);
    return encCaps.MBPerSec;
}

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE MfxOmxVencComponent::mfx2omx(
    OMX_INDEXTYPE idx,
    T* omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams,
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

template<typename T>
OMX_ERRORTYPE MfxOmxVencComponent::ValidateAndConvert(
    MfxOmxInputConfig& config,
    OMX_INDEXTYPE idx,
    const T* omxparams,
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

    config.bCodecInitialized = m_bInitialized;
    return omx2mfx_config(config, m_NextConfig, omxparams);
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::ValidateConfig(
    OMX_U32 kind,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pConfig,
    MfxOmxInputConfig & config)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorUnsupportedIndex;

    // Mutex is need to 1) avoid race condition on setting parameters and encoder
    // initialization, 2) avoid using old data in Query below.
    // We fill 'config' data in the ValidateAndConvert func but after that the valid
    // mfx data might change in other thread and we've got desync msdk.
    MfxOmxAutoLock lock(m_encoderMutex);

    switch (static_cast<int> (nIndex))
    {
    case OMX_IndexParamIntelAVCVUI:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamIntelAVCVUI");
        if (kind != eSetParameter) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_INTEL_AVCVUI*>(pConfig));
        break;
    case OMX_IndexParamVideoAvc:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoAvc");
        if (kind != eSetParameter) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_AVCTYPE*>(pConfig));
        break;
    case OMX_IndexParamVideoHevc:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoHevc");
        if (kind != eSetParameter) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_HEVCTYPE*>(pConfig));
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoProfileLevelCurrent");
        if (kind != eSetParameter) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE*>(pConfig));
        break;
    case OMX_IndexParamVideoIntraRefresh:
        // config & set
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoIntraRefresh");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pConfig));
        break;
    case OMX_IndexConfigAndroidIntraRefresh:
        // config & set
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigAndroidIntraRefresh");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE*>(pConfig));
        break;
    case OMX_IndexParamVideoBitrate:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoBitrate");
        if (kind != eSetParameter) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_BITRATETYPE*>(pConfig));
        if ((OMX_ErrorNone == omx_res) && pConfig)
        {
            m_eOmxControlRate = static_cast<OMX_VIDEO_PARAM_BITRATETYPE*>(pConfig)->eControlRate;
            if (m_pBitstreams && (OMX_Video_Intel_ControlRateVideoConferencingMode == m_eOmxControlRate))
            {
                m_pBitstreams->SetRemovingSpsPps(false);
            }
            mfxU32 nBufferSize = GetBufferSize(*config.mfxparams);
            if (nBufferSize > m_pOutPortDef->nBufferSize) m_pOutPortDef->nBufferSize = MFX_OMX_MEM_ALIGN(nBufferSize, 32);
        }
        break;
    case OMX_IndexParamNalStreamFormat:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamNalStreamFormat");
        {
            if (kind != eSetParameter) break;
            OMX_NALSTREAMFORMATTYPE* pParam = static_cast<OMX_NALSTREAMFORMATTYPE*>(pConfig);
            if (IsStructVersionValid<OMX_NALSTREAMFORMATTYPE>(pParam, sizeof(OMX_NALSTREAMFORMATTYPE), OMX_VERSION))
            {
                if (OMX_NaluFormatStartCodes == pParam->eNaluFormat) omx_res = OMX_ErrorNone;
                else omx_res = OMX_ErrorUnsupportedSetting;
            }
            else omx_res = OMX_ErrorVersionMismatch;
        }
        break;
    case OMX_IndexConfigVideoFramerate: // reset
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoFramerate");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_FRAMERATETYPE*>(pConfig));
        break;
    case OMX_IndexConfigVideoIntraVOPRefresh: // per-frame
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoIntraVOPRefresh");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_INTRAREFRESHVOPTYPE*>(pConfig));
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod: // reset
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoAVCIntraPeriod");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_AVCINTRAPERIOD*>(pConfig));
        break;
    case OMX_IndexConfigVideoBitrate: // reset
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoBitrate");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(pConfig));
        break;
    case OMX_IndexConfigIntelBitrate: // reset
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigIntelBitrate");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_INTEL_BITRATETYPE*>(pConfig));
        if ((OMX_ErrorNone == omx_res) && config.control)
        {
            m_MfxEncodeCtrl.QP = config.control->QP;
        }
        break;
    case OMX_IndexConfigIntelSliceNumbers: // reset
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigIntelSliceNumbers");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS*>(pConfig));
        break;
    case OMX_IndexExtTemporalLayerCount:
        MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtTemporalLayerCount");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT*>(pConfig));
        break;
    case MfxOmx_IndexIntelHRDParameter:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelHRDParameter");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_HRD_PARAM*>(pConfig));
        break;
    case MfxOmx_IndexIntelMaxPictureSize:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelMaxPictureSize");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_MAX_PICTURE_SIZE*>(pConfig));
        break;
    case MfxOmx_IndexIntelTargetUsage:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelTargetUsage");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_TARGET_USAGE*>(pConfig));
        break;
    case MfxOmx_IndexIntelEncoderFrameCropping:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderFrameCropping");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM*>(pConfig));
        break;
    case MfxOmx_IndexIntelEncoderVUIControl:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderVUIControl");
        if ((kind != eSetConfig) && (kind != eSetParameter)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM*>(pConfig));
        break;
    case MfxOmx_IndexIntelUserData:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelUserData");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_USERDATA*>(pConfig));
        break;
    case MfxOmx_IndexIntelEncoderDirtyRect:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderDirtyRect");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_DIRTY_RECT*>(pConfig));
        break;
    case MfxOmx_IndexIntelEncoderDummyFrame:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderDummyFrame");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_DUMMYFRAME*>(pConfig));
        break;
    case MfxOmx_IndexIntelEncoderSliceNumber:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderSliceNumber");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_NUMSLICE*>(pConfig));
        break;
    case MfxOmx_IndexIntelEnableInternalSkip:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEnableInternalSkip");
        if ((kind != eSetConfig)) break;
        if (static_cast<OMX_CONFIG_ENABLE_INTERNAL_SKIP*>(pConfig)->bEnableInternalSkip)
            m_bEnableInternalSkip = true;
        omx_res = OMX_ErrorNone;
        break;
    case MfxOmx_IndexIntelBitrateLimitOff:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelBitrateLimitOff");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_BITRATELIMITOFF*>(pConfig));
        break;
    case MfxOmx_IndexIntelNumRefFrame:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelNumRefFrame");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_NUMREFFRAME*>(pConfig));
        break;
    case MfxOmx_IndexIntelGopPicSize:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelGopPicSize");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_GOPPICSIZE*>(pConfig));
        break;
    case MfxOmx_IndexIntelIdrInterval:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelIdrInterval");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_IDRINTERVAL*>(pConfig));
        break;
    case MfxOmx_IndexIntelGopRefDist:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelGopRefDist");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_GOPREFDIST*>(pConfig));
        break;
    case MfxOmx_IndexIntelLowPower:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelLowPower");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_LOWPOWER*>(pConfig));
        break;
    case MfxOmx_IndexIntelDisableDeblockingIdc:
        MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelDisableDeblockingIdc");
        if ((kind != eSetConfig)) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_CONFIG_DISABLEDEBLOCKINGIDC*>(pConfig));
        break;
    case MFX_OMX_IndexConfigOperatingRate:
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_IndexConfigOperatingRate");
        if (kind != eSetConfig) break;
        omx_res = ValidateAndConvert(
            config,
            nIndex,
            static_cast<OMX_VIDEO_CONFIG_OPERATION_RATE*>(pConfig));
        break;
    case MFX_OMX_IndexConfigPriority:
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_IndexConfigPriority");
        if (kind != eSetConfig) break;
        m_priority = (static_cast<OMX_VIDEO_CONFIG_PRIORITY*>(pConfig)->nU32);
        m_MfxVideoParams.AsyncDepth = GetAsyncDepth();
        omx_res = OMX_ErrorNone;
        break;
    default:
        omx_res = OMX_ErrorUnsupportedIndex;
        break;
    };

    if ((OMX_ErrorNone == omx_res) && config.mfxparams)
    {
        MfxOmxVideoParamsWrapper* wrap = static_cast<MfxOmxVideoParamsWrapper*>(config.mfxparams);

        MFX_OMX_AUTO_TRACE_MSG("Checking new params");
        MFX_OMX_AT__mfxVideoParam_enc((*m_NextConfig.mfxparams));

        mfxStatus sts;
        {
            sts = m_pENC->Query(config.mfxparams, config.mfxparams);
            MFX_OMX_LOG("m_pENC->Query returned %d", sts);
        }

        if ((MFX_ERR_NONE != sts) && (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM != sts))
        {
            MFX_OMX_DELETE(config.mfxparams);
            if (config.control)
            {
                if (config.control->payload)
                {
                    MFX_OMX_DELETE(config.control->payload->Data);
                }
                MFX_OMX_DELETE(config.control->payload);
            }
            MFX_OMX_DELETE(config.control);
            omx_res = OMX_ErrorUnsupportedSetting;
        }
        else
        {
            if (!m_bInitialized)
            { // encoder was not yet initialized - we just aggregate parameters
                m_OmxMfxVideoParams = *wrap;
                MFX_OMX_DELETE(config.mfxparams);
            }
            else
            { // encoder was initialized - we validate parameters
                if (m_NextConfig.mfxparams == &m_OmxMfxVideoParams)
                {
                    MFX_OMX_AUTO_TRACE_MSG("switched to m_OmxMfxVideoParamsNext for m_NextConfig.mfxparams");
                    m_NextConfig.mfxparams = &m_OmxMfxVideoParamsNext;
                }
                m_OmxMfxVideoParamsNext = *wrap;
            }
        }
        MFX_OMX_AUTO_TRACE_MSG("m_NextConfig.params: what is...");
        MFX_OMX_AT__mfxVideoParam_enc((*m_NextConfig.mfxparams));
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::UpdateBufferCount(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
    {
        MFX_OMX_AUTO_TRACE_MSG("INPUT_PORT_INDEX");
        mfxFrameAllocRequest request = {};

        mfxStatus mfx_res = m_pENC->QueryIOSurf(&m_MfxVideoParams, &request);
        MFX_OMX_AUTO_TRACE_I32(mfx_res);

        if (MFX_ERR_NONE == mfx_res)
        {
            m_nSurfacesNum = MFX_OMX_MAX(m_nSurfacesNum,
                                         MFX_OMX_MAX(request.NumFrameSuggested, request.NumFrameMin));
            m_nSurfacesNum = MFX_OMX_MAX(m_nSurfacesNum, 2);

            m_pInPortDef->nBufferCountMin = request.NumFrameMin;
            m_pInPortDef->nBufferCountActual = m_nSurfacesNum;

            MFX_OMX_AUTO_TRACE_I32(m_pInPortDef->nBufferCountMin);
            MFX_OMX_AUTO_TRACE_I32(m_pInPortDef->nBufferCountActual);
        }
        else
        {
            MFX_OMX_AT__mfxVideoParam_enc(m_MfxVideoParams);
            omx_res = OMX_ErrorUndefined;
        }
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("OUTPUT_PORT_INDEX");
        m_pOutPortDef->nBufferCountMin = m_pInPortDef->nBufferCountMin;
        m_pOutPortDef->nBufferCountActual = m_pInPortDef->nBufferCountActual + 1;

        MFX_OMX_AUTO_TRACE_I32(m_pOutPortDef->nBufferCountMin);
        MFX_OMX_AUTO_TRACE_I32(m_pOutPortDef->nBufferCountActual);
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::DealWithBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::DealWithBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);

    if (OMX_ErrorNone == omx_res)
    {
        mfxStatus mfx_sts = MFX_ERR_NONE;
        OMX_PARAM_PORTDEFINITIONTYPE* port_def = NULL;
        MFX_OMX_PARAM_PORTINFOTYPE* port_info = NULL;
        OMX_BUFFERHEADERTYPE* pBufferHeader = NULL;

        omx_res = AllocOMXBuffer(&pBufferHeader, nPortIndex, pAppPrivate, nSizeBytes, pBuffer);
        if (OMX_ErrorNone == omx_res)
        {
            if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
            {
                MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_INPUT_PORT_INDEX");
                port_def = m_pInPortDef;
                port_info = m_pInPortInfo;

                if ((MFX_ERR_NONE == mfx_sts) && !port_info->nCurrentBuffersCount)
                {
                    mfxFrameInfo InputFrameInfo;
                    MfxVideoParams_Get_From_InPort(&(InputFrameInfo));

                    mfx_sts = m_pSurfaces->Init(&(InputFrameInfo),
                                                &(m_MfxVideoParams.mfx.FrameInfo));
                }
                if (MFX_ERR_NONE == mfx_sts)
                {
                    mfx_sts = m_pSurfaces->PrepareSurface(pBufferHeader);
                }
            }
            else
            {
                MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_OUTPUT_PORT_INDEX");
                port_def = m_pOutPortDef;
                port_info = m_pOutPortInfo;

                mfx_sts = m_pBitstreams->PrepareBitstream(pBufferHeader, &(m_MfxVideoParams.mfx));

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
                }
            }
        }
        else mfx_sts = MFX_ERR_MEMORY_ALLOC;

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
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::AllocOMXBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxStatus mfx_sts = MFX_ERR_NONE;

    // allocating buffer and its info
    OMX_BUFFERHEADERTYPE* pBufferHeader = (OMX_BUFFERHEADERTYPE*)calloc(1, sizeof(OMX_BUFFERHEADERTYPE));
    MfxOmxBufferInfo* pBufInfo = (MfxOmxBufferInfo*)calloc(1, sizeof(MfxOmxBufferInfo));
    OMX_U8* pData = NULL;

    if (!pBuffer) pData = (OMX_U8*)malloc(nSizeBytes);
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
            MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_INPUT_PORT_INDEX");

            pBufInfo->eType = MfxOmxBuffer_Surface;

            pBufferHeader->nInputPortIndex = nPortIndex;
            pBufferHeader->pInputPortPrivate = pBufInfo;
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_OUTPUT_PORT_INDEX");

            pBufInfo->eType = MfxOmxBuffer_Bitstream;

            pBufferHeader->nOutputPortIndex = nPortIndex;
            pBufferHeader->pOutputPortPrivate = pBufInfo;
        }
    }
    else mfx_sts = MFX_ERR_MEMORY_ALLOC;

    if (MFX_ERR_NONE == mfx_sts)
    {
        *ppBufferHdr = pBufferHeader;

        MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBufferHeader));
    }
    else
    {
        MFX_OMX_FREE(pBufferHeader);
        MFX_OMX_FREE(pBufInfo);
        MFX_OMX_FREE(pData);
        omx_res = OMX_ErrorInsufficientResources;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::FreeBuffer(
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::FreeBuffer(nPortIndex, pBuffer);

    if (OMX_ErrorNone == omx_res)
    {
        OMX_PARAM_PORTDEFINITIONTYPE* port_def = NULL;
        MFX_OMX_PARAM_PORTINFOTYPE* port_info = NULL;

        MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBuffer));

        omx_res = FreeOMXBuffer(nPortIndex, pBuffer);

        if (OMX_ErrorNone == omx_res)
        {
            if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
            {
                MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_INPUT_PORT_INDEX");
                port_def = m_pInPortDef;
                port_info = m_pInPortInfo;
            }
            else
            {
                MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_OUTPUT_PORT_INDEX");
                port_def = m_pOutPortDef;
                port_info = m_pOutPortInfo;
            }

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
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::FreeOMXBuffer(
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    MfxOmxBufferInfo* buf_info = NULL;

    MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBuffer));

    if (MFX_OMX_INPUT_PORT_INDEX == nPortIndex)
    {
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_INPUT_PORT_INDEX");
        buf_info = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;

        mfxStatus mfx_res = m_pSurfaces->FreeSurface(pBuffer);
        if (MFX_ERR_NONE != mfx_res)
            omx_res = OMX_ErrorUndefined;
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_OUTPUT_PORT_INDEX");
        buf_info = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;
    }
    if (OMX_ErrorNone == omx_res)
    {
        if (buf_info && buf_info->bSelfAllocatedBuffer)
        {
            MFX_OMX_FREE(pBuffer->pBuffer);
        }
        MFX_OMX_FREE(buf_info);
        MFX_OMX_FREE(pBuffer);
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::EmptyThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EmptyThisBuffer(%p) nFilledLen %d, nTimeStamp %lld, nFlags 0x%x", pBuffer, pBuffer->nFilledLen, pBuffer->nTimeStamp, pBuffer->nFlags);

    char name[25];
    snprintf(name, sizeof(name), "OMX surface:%p", pBuffer);
    ATRACE_ASYNC_BEGIN(name, (intptr_t)pBuffer);
    OMX_ERRORTYPE omx_res = MfxOmxComponent::EmptyThisBuffer(pBuffer);

    if (OMX_ErrorNone == omx_res)
    {
        MfxOmxInputData input;
        MFX_OMX_ZERO_MEMORY(input);

        input.type = MfxOmxInputData::MfxOmx_InputData_Buffer;
        input.buffer = pBuffer;

        if (m_input_queue.Add(&input)) m_pCommandsSemaphore->Post();
        else omx_res = OMX_ErrorInsufficientResources;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::FillThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "FillThisBuffer(%p) nAllocLen %d", pBuffer, pBuffer->nAllocLen);

    char name[25];
    snprintf(name, sizeof(name), "OMX bitstream:%p", pBuffer);
    ATRACE_ASYNC_BEGIN(name, (intptr_t)pBuffer);

    OMX_ERRORTYPE omx_res = MfxOmxComponent::FillThisBuffer(pBuffer);

    if (OMX_ErrorNone == omx_res)
    {
        if (MFX_ERR_NONE == m_pBitstreams->UseBuffer(pBuffer))
        {
            m_pCommandsSemaphore->Post();
        }
        else omx_res = OMX_ErrorUndefined;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}
/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::GetParameter(nParamIndex, pComponentParameterStructure);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        MfxOmxAutoLock lock(m_encoderMutex);
        switch (static_cast<int> (nParamIndex))
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
                        OMX_PARAM_PORTDEFINITIONTYPE & port = m_pPorts[pParam->nPortIndex]->m_port_def;
                        omx_res = UpdateBufferCount(pParam->nPortIndex);
                        *pParam = port;

                        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "GetParameter(ParamPortDefinition) nPortIndex %d, nBufferCountMin %d, nBufferCountActual %d, nBufferSize %d, nFrameWidth %d, nFrameHeight %d",
                                            port.nPortIndex, port.nBufferCountMin, port.nBufferCountActual, port.nBufferSize, port.format.video.nFrameWidth, port.format.video.nFrameHeight);
                        MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE(port);
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

                        if (format_index < video_reg->m_formats_num)
                        {
                            *pParam = video_reg->m_formats[format_index];
                            pParam->nIndex = format_index; // returning this value back

                            MFX_OMX_AT__OMX_VIDEO_PARAM_PORTFORMATTYPE(video_reg->m_formats[format_index]);

                            omx_res = OMX_ErrorNone;
                        }
                        else omx_res = OMX_ErrorNoMore;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGooglePrependSPSPPSToIDRFrames:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrependSPSPPSToIDRFrames");
                android::PrependSPSPPSToIDRFramesParams* pParam = (android::PrependSPSPPSToIDRFramesParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::PrependSPSPPSToIDRFramesParams>(pParam, sizeof(android::PrependSPSPPSToIDRFramesParams), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrependSPSPPSToIDRFrames is valid");
                    omx_res = OMX_ErrorNone;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamIntelAVCVUI:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamIntelAVCVUI");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_INTEL_AVCVUI*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexParamVideoAvc:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoAvc");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_AVCTYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexParamVideoHevc:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoHevc");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_HEVCTYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexParamVideoProfileLevelCurrent:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoProfileLevelCurrent");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_PROFILELEVELTYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexParamVideoBitrate:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoBitrate");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_BITRATETYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexParamVideoIntraRefresh:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoIntraRefresh");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigAndroidIntraRefresh:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigAndroidIntraRefresh");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoAVCIntraPeriod");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_CONFIG_AVCINTRAPERIOD*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigVideoBitrate:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoBitrate");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case MFX_OMX_IndexParamConsumerUsageBits:
            MFX_OMX_AUTO_TRACE_MSG("MFX_OMX_IndexParamConsumerUsageBits");
            {
                OMX_U32 *param = static_cast<OMX_U32*>(pComponentParameterStructure);
                if (param) *param = 0;
                omx_res = OMX_ErrorNone;
            }
            break;
        case MfxOmx_IndexIntelHRDParameter:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelHRDParameter");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_HRD_PARAM*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case MfxOmx_IndexIntelMaxPictureSize:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelMaxPictureSize");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_MAX_PICTURE_SIZE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case MfxOmx_IndexIntelTargetUsage:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelTargetUsage");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_TARGET_USAGE*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case MfxOmx_IndexIntelEncoderFrameCropping:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderFrameCropping");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case MfxOmx_IndexIntelEncoderVUIControl:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderVUIControl");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigIntelSliceNumbers:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigIntelSliceNumbers");
            omx_res = mfx2omx(
                nParamIndex,
                static_cast<OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS*>(pComponentParameterStructure),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexExtRequestBlackFramePointer:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexExtRequestBlackFramePointer");
            omx_res = GetBlackFrame(static_cast<OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER*>(pComponentParameterStructure));
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

OMX_ERRORTYPE MfxOmxVencComponent::SetParameter(
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::SetParameter(nParamIndex, pComponentParameterStructure);
    MfxOmxInputConfig config;

    //bool bIncorrectState = (OMX_StateLoaded != m_state) && (OMX_StateWaitForResources != m_state);
    MFX_OMX_ZERO_MEMORY(config);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (static_cast<int> (nParamIndex))
        {
        case MfxOmx_IndexGoogleMetaDataInBuffers:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: Omx_IndexGoogleMetaDataInBuffers");
                android::StoreMetaDataInBuffersParams* pParam = (android::StoreMetaDataInBuffersParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::StoreMetaDataInBuffersParams>(pParam, sizeof(android::StoreMetaDataInBuffersParams), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    // set input memory pattern for msdk encoder
                    if (MFX_OMX_INPUT_PORT_INDEX == pParam->nPortIndex)
                    {
                        MFX_OMX_AUTO_TRACE_I32(pParam->bStoreMetaData);
                        {
                            m_MfxVideoParams.IOPattern &= !(MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_IN_OPAQUE_MEMORY);
                            m_MfxVideoParams.IOPattern |= pParam->bStoreMetaData ? MFX_IOPATTERN_IN_VIDEO_MEMORY : MFX_IOPATTERN_IN_SYSTEM_MEMORY;

                            mfxU32 mode = pParam->bStoreMetaData ? MfxOmxInputSurfacesPool::MODE_LOAD_MDBUF
                                                            : MfxOmxInputSurfacesPool::MODE_LOAD_SWMEM;
                            mfxStatus mfx_res = m_pSurfaces->SetMode(mode);

                            if ( MFX_ERR_NONE == mfx_res ) omx_res = OMX_ErrorNone;
                        }
                    }
                    else
                    {
                        MFX_OMX_AUTO_TRACE_MSG("Output port bStoreMetaData parameter value is skipped by OMX plug-in now");
                        omx_res = OMX_ErrorNone;
                    }
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case MfxOmx_IndexGooglePrependSPSPPSToIDRFrames:
            {
                MFX_OMX_AUTO_TRACE_MSG("Extension Index: MfxOmx_IndexGooglePrependSPSPPSToIDRFrames");
                android::PrependSPSPPSToIDRFramesParams* pParam = (android::PrependSPSPPSToIDRFramesParams*)pComponentParameterStructure;
                if (IsStructVersionValid<android::PrependSPSPPSToIDRFramesParams>(pParam, sizeof(android::PrependSPSPPSToIDRFramesParams), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->bEnable);
                    if (m_pBitstreams != NULL)
                    {
                        m_pBitstreams->SetRemovingSpsPps(!pParam->bEnable);
                    }
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

    if (OMX_ErrorNone == omx_res)
    {
        if (OMX_COLOR_FormatAndroidOpaque == m_pPorts[MFX_OMX_INPUT_PORT_INDEX]->m_port_def.format.video.eColorFormat)
        {
            MFX_OMX_AUTO_TRACE_MSG("Component will work with Android opaque buffers.");
            m_pSurfaces->SetMode(MfxOmxInputSurfacesPool::MODE_LOAD_OPAQUE);
            mfx_omx_adjust_port_definition(&(m_pPorts[MFX_OMX_INPUT_PORT_INDEX]->m_port_def), NULL,
                                           false, false);
        }
    }
    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        omx_res = ValidateConfig(eSetParameter, nParamIndex, pComponentParameterStructure, config);
    }

    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::SetConfig(
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::SetConfig(nIndex, pComponentConfigStructure);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        switch (nIndex)
        {
        default:
            MFX_OMX_AUTO_TRACE_MSG("unknown nIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::GetConfig(
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pConfig)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = MfxOmxComponent::GetConfig(nIndex, pConfig);

    if (OMX_ErrorUnsupportedIndex == omx_res)
    {
        MfxOmxAutoLock lock(m_encoderMutex);
        switch (static_cast<int> (nIndex))
        {
        case OMX_IndexParamVideoIntraRefresh:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoIntraRefresh");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_INTRAREFRESHTYPE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigAndroidIntraRefresh:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigAndroidIntraRefresh");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigVideoFramerate:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoFramerate");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_CONFIG_FRAMERATETYPE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoAVCIntraPeriod");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_CONFIG_AVCINTRAPERIOD*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigVideoBitrate:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigVideoBitrate");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_CONFIG_BITRATETYPE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigIntelBitrate:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigIntelBitrate");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_CONFIG_INTEL_BITRATETYPE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case OMX_IndexConfigIntelSliceNumbers:
            MFX_OMX_AUTO_TRACE_MSG("OMX_IndexConfigIntelSliceNumbers");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case MfxOmx_IndexIntelHRDParameter:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelHRDParameter");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_HRD_PARAM*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        case  MfxOmx_IndexIntelMaxPictureSize:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelMaxPictureSize");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_MAX_PICTURE_SIZE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
       case MfxOmx_IndexIntelTargetUsage:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelTargetUsage");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_TARGET_USAGE*>(pConfig),
                m_OmxMfxVideoParams);
            break;
       case MfxOmx_IndexIntelEncoderFrameCropping:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderFrameCropping");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM*>(pConfig),
                m_OmxMfxVideoParams);
            break;
       case MfxOmx_IndexIntelEncoderVUIControl:
            MFX_OMX_AUTO_TRACE_MSG("MfxOmx_IndexIntelEncoderVUIControl");
            omx_res = mfx2omx(
                nIndex,
                static_cast<OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM*>(pConfig),
                m_OmxMfxVideoParams);
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("unknown nIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::GetBlackFrame(OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER* pParam)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!IsStructVersionValid<OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER>(pParam, sizeof(OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER), OMX_VERSION))
    {
        return OMX_ErrorVersionMismatch;
    }
    if (!IsIndexValid(static_cast<OMX_INDEXTYPE>(OMX_IndexExtRequestBlackFramePointer), pParam->nPortIndex))
    {
        return OMX_ErrorBadPortIndex;
    }
    if (NULL == m_blackFrame)
    {
        buffer_handle_t handle = NULL;

        MfxOmxGrallocAllocator * gralloc = m_pDevice->GetGrallocAllocator();
        mfx_res = gralloc->Alloc(m_MfxVideoParams.mfx.FrameInfo.Width, m_MfxVideoParams.mfx.FrameInfo.Height, handle);

        mfxFrameData data = {};
        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = gralloc->LockFrame(handle, &data);
        }

        if (MFX_ERR_NONE == mfx_res)
        {
            memset(data.Y, 0x0, data.Pitch * m_MfxVideoParams.mfx.FrameInfo.Height);
            memset(data.U, 0x80, data.Pitch * m_MfxVideoParams.mfx.FrameInfo.Height / 2);
            mfx_res = gralloc->UnlockFrame(handle, &data);

            m_pSurfaces->SetBlackFrame(handle);
            m_blackFrame = handle;

            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Allocated black frame handle %p", m_blackFrame);
        }
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        pParam->nFramePointer = (OMX_PTR) m_blackFrame;
        MFX_OMX_AT__OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER((*pParam));
    }

    return (MFX_ERR_NONE == mfx_res) ? OMX_ErrorNone : OMX_ErrorUndefined;
}

/*------------------------------------------------------------------------------*/

void MfxOmxVencComponent::BufferReleased(
    MfxOmxPoolId id,
    OMX_BUFFERHEADERTYPE* pBuffer,
    mfxStatus error)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(error);
    if (id == m_pSurfaces->GetPoolId())
    {
        char name[25];
        snprintf(name, sizeof(name), "OMX surface:%p", pBuffer);
        ATRACE_ASYNC_END(name, (intptr_t)pBuffer);

        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EmptyBufferDone(%p)", pBuffer);
        m_pCallbacks->EmptyBufferDone(m_self, m_pAppData, pBuffer);
    }
    else if (id == m_pBitstreams->GetPoolId())
    {
        char name[25];
        snprintf(name, sizeof(name), "OMX bitstream:%p", pBuffer);
        ATRACE_ASYNC_END(name, (intptr_t)pBuffer);

        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "FillBufferDone(%p) nFilledLen %d, nTimeStamp %lld, nFlags 0x%x",
                            pBuffer, pBuffer->nFilledLen, pBuffer->nTimeStamp, pBuffer->nFlags);
        m_pCallbacks->FillBufferDone(m_self, m_pAppData, pBuffer);
    }
}

/*------------------------------------------------------------------------------*/

/* NOTE:
 * - Possibility of the state transition is already validated on the enter
 * to this function.
 */
bool MfxOmxVencComponent::IsStateTransitionReady(void)
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
            MFX_OMX_AUTO_TRACE_MSG("unhandled state: possible BUG");
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

OMX_ERRORTYPE MfxOmxVencComponent::CommandStateSet(OMX_STATETYPE new_state)
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
                MFX_OMX_LOG_ERROR("error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        case OMX_StateIdle:
            MFX_OMX_AUTO_TRACE_MSG("OMX_StateIdle");
            if (OMX_StateInvalid == m_state)
            {
                MFX_OMX_LOG_ERROR("error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            else
            {
                m_state_to_set = OMX_StateIdle;
                if (OMX_StateExecuting == m_state)
                { // resetting codec
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
                MFX_OMX_LOG_ERROR("error: OMX_ErrorIncorrectStateTransition");
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
                MFX_OMX_LOG_ERROR("error: OMX_ErrorIncorrectStateTransition");
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
                MFX_OMX_LOG_ERROR("error: OMX_ErrorIncorrectStateTransition");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, (OMX_U32)OMX_ErrorIncorrectStateTransition, 0, NULL);
            }
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("switch to unhandled state: possible BUG");
            break;
        };
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::CommandPortDisable(OMX_U32 nPortIndex)
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
        m_pSurfaces->Close();
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
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortDisable, m_pInPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling input port completed");
    }
    if ((MFX_OMX_OUTPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling output port");
        MFX_OMX_AUTO_TRACE("Disabling output port");
        // releasing all buffers
        m_pBitstreams->Reset();
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
        m_pOutPortInfo->bDisable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortDisable, m_pOutPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Disabling output port completed");
    }
    else omx_res = OMX_ErrorBadPortIndex;
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::CommandPortEnable(OMX_U32 nPortIndex)
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
        m_bChangeOutputPortSettings = false;
        m_pOutPortInfo->bEnable = false;
        m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandPortEnable, m_pOutPortDef->nPortIndex, NULL);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Enabling output port completed");
    }
    else omx_res = OMX_ErrorBadPortIndex;
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxVencComponent::CommandFlush(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    mfxStatus mfx_sts = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    if ((MFX_OMX_INPUT_PORT_INDEX == nPortIndex) || (OMX_ALL == nPortIndex))
    {
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush input buffers");
        MFX_OMX_AUTO_TRACE("flush input buffers");

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
        MFX_OMX_AUTO_TRACE("flush output buffers");

        mfx_sts = ResetOutput();
        if (MFX_ERR_NONE == mfx_sts)
        {
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Flush output buffers completed");
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventCmdComplete, (OMX_U32)OMX_CommandFlush, MFX_OMX_OUTPUT_PORT_INDEX, NULL);
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

void MfxOmxVencComponent::MainThread(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxInputData input;
    MfxOmxCommandData& command = input.command;
    MfxOmxInputConfigAggregator aggregator;
    mfxStatus mfx_sts = MFX_ERR_NONE;

    while (1)
    {
        m_pCommandsSemaphore->Wait();
        if (m_bDestroy) break;

        MFX_OMX_AUTO_TRACE("Main Thread Loop iteration");

        MFX_OMX_ZERO_MEMORY(input);
        input.type = MfxOmxInputData::MfxOmx_InputData_None;

        m_input_queue.Get(&input);
        MFX_OMX_AUTO_TRACE_I32(input.type);

        if (input.type == MfxOmxInputData::MfxOmx_InputData_Command)
        {
            MFX_OMX_AUTO_TRACE("Got new command");
            MFX_OMX_AUTO_TRACE_I32(command.m_command);

            {
                MFX_OMX_AUTO_TRACE("Awaiting for m_pAllSyncOpFinished");
                m_pAllSyncOpFinished->Wait();
            }

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
        else if (input.type == MfxOmxInputData::MfxOmx_InputData_Config)
        {
            if (input.config.mfxparams)
            {
                MFX_OMX_AUTO_TRACE_MSG("Trying to reset encoder");
                MfxOmxAutoLock lock(m_encoderMutex);
                m_Error = ReinitCodec(input.config.mfxparams);
                MFX_OMX_DELETE(input.config.mfxparams);
            }
            aggregator += input.config;
        }
        else if (input.type == MfxOmxInputData::MfxOmx_InputData_Buffer)
        {
            m_Error = m_pSurfaces->UseBuffer(input.buffer, aggregator.release());
        }

        if ((OMX_StateExecuting == m_state) && (MFX_ERR_NONE == m_Error) && ArePortsEnabled(OMX_ALL) && CanProcess())
        {
            MFX_OMX_AUTO_TRACE_MSG("Trying to process");

            // firstly we try to process everything left unprocessed
            if (m_bCanNotProcess)
            {
                MFX_OMX_AUTO_TRACE_MSG("finishing jobs left behind because of lack of resources");
                m_bCanNotProcess = false;
                mfx_sts = ProcessUnfinishedJobs();
                // at this point m_bCanNotProcess can be raised again: that's quite possible...
                if (MFX_ERR_NONE != mfx_sts && MFX_ERR_MORE_DATA != mfx_sts) m_Error = mfx_sts;
            }
            // secondly we are processing new input buffer if possible
            if ((MFX_ERR_NONE == m_Error) && !m_bCanNotProcess && !m_bEosHandlingStarted)
            {
                mfx_sts = ProcessBuffer();
                // at this point m_bCanNotProcess can be raised again
                if (MFX_ERR_NONE != mfx_sts && MFX_ERR_MORE_DATA != mfx_sts) m_Error = mfx_sts;
            }
            // and finally we handle EOS if we reached it
            if ((MFX_ERR_NONE == m_Error) && !m_bChangeOutputPortSettings && !m_bCanNotProcess && m_bEosHandlingStarted && !m_bEosHandlingFinished)
            {
                MFX_OMX_AUTO_TRACE("handling EOS");
                mfx_sts = ProcessEOS();
                // If error happened at the end of stream we skip this
            }
        }

        // Error processing (Callback)
        if (MFX_ERR_NONE != m_Error)
        {
            MFX_OMX_LOG_ERROR("Sending ErrorEvent to OMAX client because of error in component");
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "MainThread : m_Error = %d", m_Error);
            MFX_OMX_AUTO_TRACE_I32(m_Error);
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(m_Error), 0 , NULL);
        }
    }
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ProcessUnfinishedJobs(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    // if we ran out of output bitstreams we are trying to encode what was not encoded
    mfx_res = ProcessFrameEnc();
    if (MFX_ERR_MORE_DATA == mfx_res) mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingStarted);
    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingFinished);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ProcessBuffer(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    {
        MfxOmxAutoLock lock(m_encoderMutex);
        if (!m_bInitialized) mfx_res = InitEncoder();
        if ((MFX_ERR_NONE == mfx_res) && !m_bVppDetermined) mfx_res = InitVPP();
    }
    if ((MFX_ERR_NONE == mfx_res) && !m_bChangeOutputPortSettings && m_bVppDetermined)
    {
        if (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId ||
            MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId)
        {
            MfxOmxAutoLock lock(m_encoderMutex);
            if (!m_bCodecDataSent)
            {
                mfx_res = SendCodecData();
                if (MFX_ERR_NONE == mfx_res) m_bCodecDataSent = true;
            }
        }

        if (MFX_ERR_NONE == mfx_res) mfx_res = ProcessFrameEnc();
    }

    if (MFX_ERR_MORE_DATA == mfx_res) mfx_res = MFX_ERR_NONE;
    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingStarted);
    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingFinished);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ProcessEOS(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = ProcessFrameEnc();
    if ((MFX_ERR_NONE != mfx_res) && (MFX_ERR_MORE_SURFACE != mfx_res)) // normal EOS is MFX_ERR_MORE_DATA
    {
        // sending special buffer which marks EOS
        mfxBitstream *pBitstream = m_pBitstreams->GetBuffer();

        if (pBitstream)
        {
            mfx_res = m_pBitstreams->QueueBufferForSending(pBitstream, NULL);
            m_bEosHandlingStarted = false;
            m_bEosHandlingFinished = true;

            m_pAsyncSemaphore->Post();
        }
        else m_bCanNotProcess = true;

        mfx_res = MFX_ERR_NONE;
    }
    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingStarted);
    MFX_OMX_AUTO_TRACE_I32(m_bEosHandlingFinished);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::InitEncoder(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    // Encoder initialization
    if (MFX_ERR_NONE == mfx_res)
    {
        if (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId)
        {
            int idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION);
            if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM) m_OmxMfxVideoParams.ext_buf[idx].opt.MaxDecFrameBuffering = m_MfxVideoParams.mfx.NumRefFrame;

            idx = m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
            if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM) m_OmxMfxVideoParams.ext_buf[idx].opt2.SkipFrame = MFX_SKIPFRAME_BRC_ONLY;

            m_OmxMfxVideoParams.enableExtParam(MFX_EXTBUFF_AVC_TEMPORAL_LAYERS);
        }

        MFX_OMX_AT__mfxVideoParam_enc(m_MfxVideoParams);

        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "AsyncDepth %d", m_MfxVideoParams.AsyncDepth);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Encoder initialization ...");
        mfx_res = m_pENC->Init(&m_MfxVideoParams);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Encoder initialized with sts %d", mfx_res);

        if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("[WRN] MFX_WRN_INCOMPATIBLE_VIDEO_PARAM was received.");
            mfx_res = MFX_ERR_NONE;
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = m_pENC->GetVideoParam(&m_MfxVideoParams);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);

    if (MFX_ERR_NONE == mfx_res)
    {
        m_bInitialized = true;

        if (m_pOutPortDef->nBufferSize < m_MfxVideoParams.mfx.BufferSizeInKB * 1000 * m_MfxVideoParams.mfx.BRCParamMultiplier)
        {
            m_bChangeOutputPortSettings = true;
            MfxVideoParams_2_PortsParams();

            m_pBitstreams->Reset();
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "requesting change of output port settings");
            m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, 0, NULL);
        }
    }
    else
    {
        CloseCodec();
    }

    MFX_OMX_AT__mfxVideoParam_enc(m_MfxVideoParams);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::InitVPP(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfxFrameSurface1 *pSrf = m_pSurfaces->GetCustomBuffer();

    MFX_OMX_AUTO_TRACE_P(pSrf);
    if (pSrf)
    {
        MFX_OMX_AUTO_TRACE_I32(pSrf->Info.FourCC);
        if (MFX_FOURCC_RGB4 == pSrf->Info.FourCC)
        {
            MfxOmxVppWrappParam param;

            param.session = &m_Session;
            param.frame_info = &(pSrf->Info);
            param.allocator = m_pDevice->GetFrameAllocator();
            param.conversion = ARGB_TO_NV12;

            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "VPP initialization ...");
            mfx_res = m_VPP.Init(&param);
            MFX_OMX_AUTO_TRACE_I32(mfx_res);
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "VPP initialized with sts %d", mfx_res);
            m_inputVppType = param.conversion;
        }
        else
        {
            m_inputVppType = CONVERT_NONE;
        }

        if (MFX_ERR_NONE == mfx_res) m_bVppDetermined = true;
        else CloseCodec();
    }
    else if (m_pSurfaces->WasEosReached())
        m_bVppDetermined = true;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ReinitCodec(MfxOmxVideoParamsWrapper* wrap)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Reinit encoder+");
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_pAllSyncOpFinished->Wait();

    MFX_OMX_AT__mfxVideoParam_enc((*wrap));

    mfx_res = m_pENC->Reset(wrap);
    if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("[WRN] MFX_WRN_INCOMPATIBLE_VIDEO_PARAM was received.");
        mfx_res = MFX_ERR_NONE;
    }
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("failed to reset codec, trying to roll back");

        mfx_res = m_pENC->Reset(&m_MfxVideoParams);
        if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("[WRN] MFX_WRN_INCOMPATIBLE_VIDEO_PARAM was received.");
            mfx_res = MFX_ERR_NONE;
        }
        if (MFX_ERR_NONE != mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("failed to roll back reset, probably we will die");
            return mfx_res;
        }
    }
    // getting new parameters
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = m_pENC->GetVideoParam(&m_MfxVideoParams);
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        m_OmxMfxVideoParamsNext = m_OmxMfxVideoParams;

        mfxU32 requiredSize = m_MfxVideoParams.mfx.BufferSizeInKB * 1000 * m_MfxVideoParams.mfx.BRCParamMultiplier;

        if (m_pOutPortDef->nBufferSize < requiredSize)
        {
            // if a previous EventPortSettingsChanged procedure hasn't been complited
            // then postone sending a new one until next ProcessFrameEnc()
            if (!m_bChangeOutputPortSettings)
            {
                m_bChangeOutputPortSettings = true;
                MfxVideoParams_2_PortsParams();
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "requesting change of output port settings");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, 0, NULL);
            }
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Reinit encoder-");
    MFX_OMX_AT__mfxVideoParam_enc(m_MfxVideoParams);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ResetInput(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = m_pSurfaces->Reset();

    m_lastTimeStamp = 0xFFFFFFFFFFFFFFFF;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ResetOutput(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (m_pENC)
    {
        MfxOmxAutoLock lock(m_encoderMutex);
        mfx_res = m_pENC->Reset(&m_MfxVideoParams);
    }

    if ((MFX_ERR_NOT_INITIALIZED == mfx_res) || (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res))
    {
        mfx_res = MFX_ERR_NONE;
    }

    m_pBitstreams->Reset();

    m_bCanNotProcess = false;
    m_bEosHandlingStarted = false;
    m_bEosHandlingFinished = false;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::ResetCodec(void)
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

void MfxOmxVencComponent::CloseCodec(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (m_pENC) m_pENC->Close();
    m_pSurfaces->Close();
    m_pBitstreams->Reset();

    do
    {
        MFX_OMX_FREE(m_pFreeSyncPoint);
    }
    while (m_SyncPoints.Get(&m_pFreeSyncPoint, g_NilSyncPoint));

    // return parameters to initial state
    m_bInitialized = false;
    m_nSurfacesNum = 0;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxVencComponent::CanProcess(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (m_bChangeOutputPortSettings) return false;
    if (!m_pBitstreams->GetBuffer())
    {
        MFX_OMX_AUTO_TRACE_MSG("Free bitstream not found");
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

mfxStatus MfxOmxVencComponent::ProcessFrameEnc(bool bHandleEos)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(bHandleEos);
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxFrameSurface1* pFrameSurface = NULL;
    mfxBitstream* pBitstream = NULL;
    mfxSyncPoint* pSyncPoint = NULL;
    mfxEncodeCtrl* pEncodeCtrl = NULL;
    MfxOmxInputConfig* config = NULL;

    pFrameSurface = m_pSurfaces->GetCustomBuffer();
    pBitstream = m_pBitstreams->GetBuffer();

    m_bEosHandlingStarted = m_pSurfaces->WasEosReached();
    if (pFrameSurface || m_bEosHandlingStarted)
    {
        // ontaining resources required for processing
        if (!m_bCanNotProcess && !pBitstream)
        {
            MFX_OMX_AUTO_TRACE_MSG("Free bitstram not found");
            m_bCanNotProcess = true;
        }
        if (!m_bCanNotProcess)
        {
            mfxU32 requiredSize = m_MfxVideoParams.mfx.BufferSizeInKB * 1000 * m_MfxVideoParams.mfx.BRCParamMultiplier;
            if (pBitstream->DataOffset + pBitstream->DataLength + requiredSize > pBitstream->MaxLength)
            {
                m_bCanNotProcess = true;
                m_bChangeOutputPortSettings = true;
                MfxVideoParams_2_PortsParams();
                MFX_OMX_AUTO_TRACE_MSG("requesting change of output port settings");
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventPortSettingsChanged, MFX_OMX_OUTPUT_PORT_INDEX, 0, NULL);
            }
        }
        if (!m_bCanNotProcess && !m_pFreeSyncPoint && !m_SyncPoints.Get(&m_pFreeSyncPoint, g_NilSyncPoint))
        {
            m_bCanNotProcess = true;
            MFX_OMX_AUTO_TRACE_MSG("Free sync point not found!"); // should never occur !!!
        }
        else pSyncPoint = m_pFreeSyncPoint;
        // processing if resources are available
        if (!m_bCanNotProcess)
        {
            if (pFrameSurface) pFrameSurface->Data.FrameOrder = m_nEncoderInputSurfacesCount;
            mfxFrameSurface1* pSurfaceToEncode = pFrameSurface;

            bool bInternalSurfaceIsEncoded = false;

            if (CONVERT_NONE != m_inputVppType && pFrameSurface)
            {
                bInternalSurfaceIsEncoded = true;
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "ProcessFrameVpp+");
                mfx_res = m_VPP.ProcessFrameVpp(pFrameSurface, &pSurfaceToEncode);
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "ProcessFrameVpp-");
            }

            if (MFX_ERR_NONE == mfx_res)
            {
                config = m_pSurfaces->GetInputConfig();
                if (config && config->control) pEncodeCtrl = config->control;
            }

            if (MFX_CODEC_AVC == m_MfxVideoParams.mfx.CodecId)
            {
                mfxU16 nSkippedFrames = 0;
                if (MFX_ERR_NONE == mfx_res)
                {
                    if (pSurfaceToEncode) nSkippedFrames = CalcNumSkippedFrames(pSurfaceToEncode->Data.TimeStamp);
                }

                if (nSkippedFrames || (m_MfxEncodeCtrl.QP != 0))
                {
                    if (NULL == pEncodeCtrl)
                    {
                        if (config) pEncodeCtrl = config->control = CreateEncodeCtrlWrapper();
                        if (NULL == pEncodeCtrl) mfx_res = MFX_ERR_NULL_PTR;
                    }
                }
                if (nSkippedFrames && pEncodeCtrl)
                {
                    mfxI32 idx = config->control->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
                    if (idx >= 0) config->control->ext_buf[idx].opt2.SkipFrame = MFX_SKIPFRAME_BRC_ONLY;
                    pEncodeCtrl->SkipFrame = nSkippedFrames;
                }

                if (m_bEnableInternalSkip && m_bSkipThisFrame)
                {
                    if (NULL == pEncodeCtrl)
                    {
                         if (config) pEncodeCtrl = config->control = CreateEncodeCtrlWrapper();
                         if (NULL == pEncodeCtrl) mfx_res = MFX_ERR_NULL_PTR;
                    }
                    if (MFX_ERR_NONE == mfx_res)
                    {
                        mfxI32 idx = config->control->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
                        if (idx >= 0)
                            config->control->ext_buf[idx].opt2.SkipFrame = MFX_SKIPFRAME_INSERT_DUMMY;
                        pEncodeCtrl->SkipFrame = true;
                    }
                    m_bSkipThisFrame = false;
                }

                if (pEncodeCtrl && (0 == pEncodeCtrl->QP) && (m_MfxEncodeCtrl.QP != 0))
                {
                    pEncodeCtrl->QP = m_MfxEncodeCtrl.QP;
                }

                if (bInternalSurfaceIsEncoded)
                {
                    // take ownership of EncodeCtrlWrap
                    if (config)
                    {
                        m_VPP.SetEncodeCtrl(config->control, pSurfaceToEncode);
                        config->control = NULL;
                    }
                }
            }

            if (MFX_ERR_NONE == mfx_res)
            {
                ATRACE_BEGIN("Submit Encode task");
                do
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EncodeFrameAsync+");
                    mfx_res = m_pENC->EncodeFrameAsync(pEncodeCtrl, pSurfaceToEncode, pBitstream, pSyncPoint);
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "EncodeFrameAsync- sts %d", mfx_res);
                    if (MFX_WRN_DEVICE_BUSY == mfx_res)
                    {
                        mfxStatus mfx_sts = MFX_ERR_NONE;
                        MFX_OMX_AUTO_TRACE("mfx(EncodeFrameAsync)::MFX_WRN_DEVICE_BUSY");
                        mfxSyncPoint* pDevBusySyncPoint = m_pBitstreams->GetSyncPoint();
                        if (pDevBusySyncPoint)
                        {
                            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation+");
                            mfx_sts = m_Session.SyncOperation(*pDevBusySyncPoint, MFX_OMX_INFINITE);
                            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation- sts %d", mfx_sts);
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
                ATRACE_END();
            }
            // valid cases for the status are:
            // MFX_ERR_NONE - data processed, output will be generated
            // MFX_ERR_MORE_DATA - data buffered, output will not be generated
            // MFX_WRN_INCOMPATIBLE_VIDEO_PARAM - frame info is not synchronized with initialization one

            // status correction
            if (MFX_WRN_INCOMPATIBLE_VIDEO_PARAM == mfx_res) mfx_res = MFX_ERR_NONE;

            if ((MFX_ERR_NONE == mfx_res) || (MFX_ERR_MORE_DATA == mfx_res))
            {
                ++m_nEncoderInputSurfacesCount;
                if (pSurfaceToEncode) m_lastTimeStamp = pSurfaceToEncode->Data.TimeStamp;

                if (MFX_ERR_NONE == mfx_res)
                {
                    ++m_nEncoderOutputBitstreamsCount;
                    MFX_OMX_AUTO_TRACE_I32(m_nEncoderOutputBitstreamsCount);

                    MFX_OMX_AUTO_TRACE_P(pSyncPoint);
                    mfx_res = m_pBitstreams->QueueBufferForSending(pBitstream, pSyncPoint);

                    if (MFX_ERR_NONE == mfx_res)
                    {
                        m_pFreeSyncPoint = NULL;

                        if (MFX_ERR_NONE == mfx_res)
                        {
                            m_pAllSyncOpFinished->Reset();
                            m_pAsyncSemaphore->Post();
                        }
                    }
                }
                if (pFrameSurface)
                {
                    mfx_res = m_pSurfaces->KeepBuffer(pFrameSurface);
                }
            }
            else
            {
                MFX_OMX_LOG_ERROR("EncodeFrameAsync failed - %s", mfx_omx_code_to_string(mfx_res));
            }
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxVencComponent::SendCodecData(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfxBitstream* pBitstream = NULL;

    pBitstream = m_pBitstreams->GetBuffer();

    mfxExtCodingOptionSPSPPS spspps;
    MFX_OMX_ZERO_MEMORY(spspps);
    mfxExtCodingOptionVPS vps;
    MFX_OMX_ZERO_MEMORY(vps);

    mfxU8 buf[1024] = {0};
    mfxExtBuffer* tmp_pExtBuf = &vps.Header;

    if (MFX_CODEC_HEVC == m_MfxVideoParams.mfx.CodecId)
    {
        mfxVideoParam tmp_par;
        MFX_OMX_ZERO_MEMORY(tmp_par);

        tmp_pExtBuf = &vps.Header;

        vps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_VPS;
        vps.Header.BufferSz = sizeof(mfxExtCodingOptionVPS);
        vps.VPSBuffer = buf;
        vps.VPSBufSize = 512;

        tmp_par.NumExtParam = 1;
        tmp_par.ExtParam = &tmp_pExtBuf;

        mfx_res = m_pENC->GetVideoParam(&tmp_par);
        if (MFX_ERR_NONE == mfx_res && pBitstream)
        {
            if (vps.VPSBufSize <= pBitstream->MaxLength - (pBitstream->DataOffset + pBitstream->DataLength))
            {
                std::copy(vps.VPSBuffer, vps.VPSBuffer + vps.VPSBufSize, pBitstream->Data + pBitstream->DataOffset + pBitstream->DataLength);
                pBitstream->DataLength += vps.VPSBufSize;
            }
            else
                mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        mfxVideoParam tmp_par;
        MFX_OMX_ZERO_MEMORY(tmp_par);

        tmp_pExtBuf = &spspps.Header;

        spspps.Header.BufferId = MFX_EXTBUFF_CODING_OPTION_SPSPPS;
        spspps.Header.BufferSz = sizeof(mfxExtCodingOptionSPSPPS);
        spspps.SPSBuffer = buf;
        spspps.SPSBufSize = 512;
        spspps.PPSBuffer = buf + spspps.SPSBufSize;
        spspps.PPSBufSize = 512;

        tmp_par.NumExtParam = 1;
        tmp_par.ExtParam = &tmp_pExtBuf;

        mfx_res = m_pENC->GetVideoParam(&tmp_par);
    }
    if (MFX_ERR_NONE == mfx_res && pBitstream)
    {
        if (spspps.SPSBufSize <= pBitstream->MaxLength - (pBitstream->DataOffset + pBitstream->DataLength))
        {
            std::copy(spspps.SPSBuffer, spspps.SPSBuffer + spspps.SPSBufSize, pBitstream->Data + pBitstream->DataOffset + pBitstream->DataLength);
            pBitstream->DataLength += spspps.SPSBufSize;
        }
        else
            mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
    }
    if (MFX_ERR_NONE == mfx_res && pBitstream)
    {
        if (spspps.PPSBufSize <= pBitstream->MaxLength - (pBitstream->DataOffset + pBitstream->DataLength))
        {
            std::copy(spspps.PPSBuffer, spspps.PPSBuffer + spspps.PPSBufSize, pBitstream->Data + pBitstream->DataOffset + pBitstream->DataLength);
            pBitstream->DataLength += spspps.PPSBufSize;
        }
        else
            mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
    }
    if (MFX_ERR_NONE == mfx_res && pBitstream)
    {
        pBitstream->DataFlag = MFX_BITSTREAM_SPSPPS;
        mfx_res = m_pBitstreams->QueueBufferForSending(pBitstream, NULL);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/
void MfxOmxVencComponent::SkipFrame(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    //skip frame algorithm in IL layer{
    MfxOmxBufferInfo* pAddBufInfo2 = MfxOmxGetOutputBufferInfo(m_pBitstreams->GetOutputBuffer());

    if (pAddBufInfo2)
    {
        //For target bitrate dynamic change
        if (m_lCurTargetBitrate != m_MfxVideoParams.mfx.TargetKbps * 1000)
        {
            m_lBufferFullness = 0;
            m_lCurTargetBitrate = m_MfxVideoParams.mfx.TargetKbps * 1000;
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "[SkipFrameLog] ###Target bitrate change.###");
        }
        mfxI64 avgFrameSize = m_MfxVideoParams.mfx.TargetKbps * 1000 * m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD / m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN;
        m_lBufferFullness += pAddBufInfo2->sBitstream.DataLength * 8 - avgFrameSize;

        mfxI64 overFlowBound = 0.8 * m_MfxVideoParams.mfx.TargetKbps * 1000;
        mfxI64 underFlowBound = -2 * m_MfxVideoParams.mfx.TargetKbps * 1000;

        if (m_lBufferFullness >= overFlowBound)
        {
            m_bSkipThisFrame = true;
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "[SkipFrameLog] ###Next frame will be skipped.###");
        }
        if (m_lBufferFullness <= underFlowBound)
        {
            m_lBufferFullness = underFlowBound;
            MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "[SkipFrameLog] ###iBufferFullness under flow bounded.###");
        }
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel,
                            "[SkipFrameLog] overFlowBound = %lld, underFlowBound = %lld, bufferFullness = %lld, "
                            "curTargetBitrate = %lld, curFrameRate = %d, avgFrameSize = %lld",
                            overFlowBound, underFlowBound, m_lBufferFullness, m_lCurTargetBitrate,
                            m_MfxVideoParams.mfx.FrameInfo.FrameRateExtN / m_MfxVideoParams.mfx.FrameInfo.FrameRateExtD, avgFrameSize);
    }
}

/*------------------------------------------------------------------------------*/
void MfxOmxVencComponent::AsyncThread(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_sts = MFX_ERR_NONE;
    mfxSyncPoint* pSyncPoint = NULL;
    OMX_BUFFERHEADERTYPE* pSPSPPS = NULL;
    MfxOmxBufferInfo* pAddBufInfo = NULL;

    while (1)
    {
        m_pAsyncSemaphore->Wait();

        if (m_bDestroy) break;

        MFX_OMX_AUTO_TRACE("Async Thread Loop iteration");

        if (MFX_ERR_NONE == mfx_sts && m_pBitstreams != NULL)
        {
            pAddBufInfo = MfxOmxGetOutputBufferInfo(m_pBitstreams->GetOutputBuffer());
            if (pAddBufInfo && MFX_BITSTREAM_SPSPPS == pAddBufInfo->sBitstream.DataFlag)
            {
                pSPSPPS = m_pBitstreams->DequeueOutputBufferForSending();
            }
            pSyncPoint = m_pBitstreams->GetSyncPoint();
            MFX_OMX_AUTO_TRACE_P(pSyncPoint);
            if (pSyncPoint)
            {
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation+");
                    ATRACE_NAME("Wait Encode task completion");
                    mfx_sts = m_Session.SyncOperation(*pSyncPoint, MFX_OMX_INFINITE);
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SyncOperation- sts %d", mfx_sts);
                    // sending event that some resources may be free
                    m_pDevBusyEvent->Signal();
                }
                if (MFX_ERR_NONE == mfx_sts)
                {
                    if (!m_SyncPoints.Add(&pSyncPoint)) mfx_sts = MFX_ERR_UNKNOWN;
                }
                else
                {
                    MFX_OMX_LOG_ERROR("SyncOperation failed - %s", mfx_omx_code_to_string(mfx_sts));
                }

                if (MFX_ERR_NONE == mfx_sts && m_pBitstreams != NULL)
                {
                    if (pSPSPPS) { m_pBitstreams->SendBitstream(pSPSPPS); pSPSPPS = NULL; }
                    if (m_bEnableInternalSkip)
                        SkipFrame();

                    m_pBitstreams->SendBitstream(m_pBitstreams->DequeueOutputBufferForSending());
                }
                if (MFX_ERR_NONE == mfx_sts && m_pSurfaces != NULL)
                {
                    m_pSurfaces->CheckBuffers();
                }
            }
            else
            {
                if (pSPSPPS) { m_pBitstreams->SendBitstream(pSPSPPS); pSPSPPS = NULL; }

                MFX_OMX_AUTO_TRACE_MSG("sending EOS frame and event");
                m_pBitstreams->SendBitstream(m_pBitstreams->DequeueOutputBufferForSending());
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventBufferFlag, MFX_OMX_OUTPUT_PORT_INDEX, OMX_BUFFERFLAG_EOS, NULL);
            }

            // Error processing (Callback)
            if (MFX_ERR_NONE != mfx_sts)
            {
                m_Error = mfx_sts;
                MFX_OMX_LOG_ERROR("Sending ErrorEvent to OMAX client because of error in component");
                MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "AsyncThread : m_Error = %d", m_Error);
                MFX_OMX_AUTO_TRACE_I32(m_Error);
                m_pCallbacks->EventHandler(m_self, m_pAppData, OMX_EventError, ErrorStatusMfxToOmx(m_Error), 0 , NULL);
            }
        }

        if ((NULL == m_pBitstreams) || (m_pBitstreams && (NULL == m_pBitstreams->GetSyncPoint())) || (MFX_ERR_NONE != m_Error))
        {
            MFX_OMX_AUTO_TRACE_MSG("Sending m_pAllSyncOpFinished");
            m_pAllSyncOpFinished->Signal();
        }
    }
}

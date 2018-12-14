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

#ifndef __MFX_OMX_VENC_COMPONENT_H__
#define __MFX_OMX_VENC_COMPONENT_H__

#include "mfx_omx_component.h"
#include "mfx_omx_vpp_wrapp.h"
#include "mfx_omx_srf_ibuf.h"
#include "mfx_omx_bst_pool.h"
#include "mfx_omx_dev.h"

/*------------------------------------------------------------------------------*/

class MfxOmxVencComponent : public MfxOmxComponent,
                            public MfxOmxBuffersCallback<OMX_BUFFERHEADERTYPE>
{
public:
    static MfxOmxComponent* Create(
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);
    virtual ~MfxOmxVencComponent(void);

    // MfxOmxComponent methods
    virtual OMX_ERRORTYPE DealWithBuffer(
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer); // if pBuffer is not NULL than call is from UseBuffer, overwise - from AllocateBuffer
    virtual OMX_ERRORTYPE FreeBuffer(
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    virtual OMX_ERRORTYPE EmptyThisBuffer(
        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
    virtual OMX_ERRORTYPE FillThisBuffer(
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    virtual OMX_ERRORTYPE SetParameter(
        OMX_IN OMX_INDEXTYPE nParamIndex,
        OMX_IN OMX_PTR pComponentParameterStructure);
    virtual OMX_ERRORTYPE GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);
    virtual OMX_ERRORTYPE SetConfig(
        OMX_IN OMX_INDEXTYPE nIndex,
        OMX_IN OMX_PTR pComponentConfigStructure);
    virtual OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);

    // MfxOmxBuffersCallback<OMX_BUFFERHEADERTYPE> methods
    virtual void BufferReleased(
        MfxOmxPoolId id,
        OMX_BUFFERHEADERTYPE* pBuffer,
        mfxStatus error);

protected:
    MfxOmxVencComponent(
        OMX_ERRORTYPE &error,
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);
    virtual OMX_ERRORTYPE Init(void);

    virtual OMX_ERRORTYPE Set_PortDefinition(OMX_PARAM_PORTDEFINITIONTYPE* pPortDef);
    virtual OMX_ERRORTYPE Set_VideoPortFormat(OMX_VIDEO_PARAM_PORTFORMATTYPE* pVideoFormat);

    virtual void MainThread(void);
    virtual void AsyncThread(void);

    virtual OMX_ERRORTYPE ValidateConfig(
        OMX_U32 kind,
        OMX_INDEXTYPE nIndex,
        OMX_PTR pComponentConfigStructure,
        MfxOmxInputConfig & config);
    virtual OMX_ERRORTYPE UpdateBufferCount(OMX_U32 nPortIndex);

    // mfx -> omx
    template<typename T>
    inline OMX_ERRORTYPE mfx2omx(
        OMX_INDEXTYPE idx,
        T* omxparams,
        const MfxOmxVideoParamsWrapper& mfxparams,
        OMX_U32 omxversion = OMX_VERSION);

    // omx -> mfx (SetConfig)
    template<typename T>
    inline OMX_ERRORTYPE ValidateAndConvert(
        MfxOmxInputConfig& config,
        OMX_INDEXTYPE idx,
        const T* omxparams,
        OMX_U32 omxversion = OMX_VERSION);

    void Reset(void);
    OMX_ERRORTYPE InPortParams_2_OutPortParams(void);
    OMX_ERRORTYPE PortsParams_2_MfxVideoParams(void);
    OMX_ERRORTYPE MfxVideoParams_2_PortsParams(void);
    OMX_ERRORTYPE MfxVideoParams_Get_From_InPort(mfxFrameInfo* out_pMfxFrameInfo);
    OMX_ERRORTYPE GetBlackFrame(OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER* pRequest);

    bool IsStateTransitionReady(void);
    OMX_ERRORTYPE CommandStateSet(OMX_STATETYPE new_state);
    OMX_ERRORTYPE CommandPortDisable(OMX_U32 nPortIndex);
    OMX_ERRORTYPE CommandPortEnable(OMX_U32 nPortIndex);
    OMX_ERRORTYPE CommandFlush(OMX_U32 nPortIndex);

    mfxStatus InitEncoder(void);
    mfxStatus InitVPP(void);
    mfxStatus ReinitCodec(MfxOmxVideoParamsWrapper* wrap);
    mfxStatus ResetInput(void);
    mfxStatus ResetOutput(void);
    mfxStatus ResetCodec(void);

    mfxStatus ProcessUnfinishedJobs(void);
    mfxStatus ProcessBuffer(void);
    mfxStatus ProcessFrameEnc(bool bHandleEos = false);
    mfxStatus ProcessEOS(void);
    mfxStatus SendCodecData(void);

    void CloseCodec(void);
    bool CanProcess(void);

    virtual mfxU16 GetAsyncDepth(void);

    bool IsMiracastMode(void);
    bool IsChromecastMode(void);

    mfxU16 CalcNumSkippedFrames(mfxU64 currentTimeStamp);
    mfxU32 QueryMaxMbPerSec(void);

    OMX_ERRORTYPE AllocOMXBuffer(
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);
    OMX_ERRORTYPE FreeOMXBuffer(
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    void SkipFrame(void);
protected:
    mfxIMPL m_Implementation;
    MFXVideoSession m_Session;
    MFXVideoENCODE* m_pENC;
    MfxOmxMutex m_encoderMutex;

    MfxOmxVideoParamsWrapper m_OmxMfxVideoParams;
    MfxOmxVideoParamsWrapper m_OmxMfxVideoParamsNext;
    mfxVideoParam& m_MfxVideoParams;

    MfxOmxEncodeCtrlWrapper m_OmxMfxEncodeCtrl;
    mfxEncodeCtrl& m_MfxEncodeCtrl;

    MfxOmxInputConfig m_NextConfig;

    OMX_VIDEO_CONTROLRATETYPE m_eOmxControlRate;

    bool m_bInitialized;
    bool m_bChangeOutputPortSettings;
    bool m_bCanNotProcess;
    bool m_bEosHandlingStarted;
    bool m_bEosHandlingFinished;
    bool m_bCodecDataSent;
    bool m_bVppDetermined;
    bool m_bSkipThisFrame;
    bool m_bEnableInternalSkip;
    mfxI64 m_lBufferFullness;
    mfxI64 m_lCurTargetBitrate;

    MfxOmxDev* m_pDevice;
    MfxOmxVppWrapp m_VPP;
    MfxOmxConversion m_inputVppType;

    MfxOmxBitstreamsPool* m_pBitstreams;

    mfxU32 m_nSurfacesNum;
    MfxOmxInputSurfacesPool* m_pSurfaces;

    MfxOmxRing<mfxSyncPoint*> m_SyncPoints;
    mfxSyncPoint* m_pFreeSyncPoint;

    mfxU32 m_nEncoderInputSurfacesCount;
    mfxU32 m_nEncoderOutputBitstreamsCount;

    buffer_handle_t m_blackFrame;

    mfxU64 m_lastTimeStamp;

    mfxU32 m_priority;

    // debug files
    FILE* m_dbg_encin;
    FILE* m_dbg_encout;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVencComponent)
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_VENC_COMPONENT_H__

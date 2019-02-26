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

#ifndef __MFX_OMX_VDEC_COMPONENT_H__
#define __MFX_OMX_VDEC_COMPONENT_H__

#include "mfx_omx_component.h"
#include "mfx_omx_bst_ibuf.h"
#include "mfx_omx_srf_pool.h"
#include "mfx_omx_dev.h"
#include "mfx_omx_color_aspects_wrapper.h"

/*------------------------------------------------------------------------------*/

#define MFX_EXTBUFF_DEC_ADAPTIVE_PLAYBACK MFX_MAKEFOURCC('A','P','B','K')

enum MfxInitState {
    MFX_INIT_NOT_INITIALIZED = 0,
    MFX_INIT_DECODE_HEADER,
    MFX_INIT_QUERY_IO_SURF,
    MFX_INIT_DECODER,
    MFX_INIT_COMPLETED
};

/*------------------------------------------------------------------------------*/

class MfxOmxVdecComponent : public MfxOmxComponent,
                            public MfxOmxBuffersCallback<OMX_BUFFERHEADERTYPE>
{
public:
    static MfxOmxComponent* Create(
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);
    virtual ~MfxOmxVdecComponent(void);

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
    virtual OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);
    virtual OMX_ERRORTYPE SetConfig(
        OMX_IN OMX_INDEXTYPE nIndex,
        OMX_IN OMX_PTR pComponentConfigStructure);

    // MfxOmxBuffersCallback<OMX_BUFFERHEADERTYPE> methods
    virtual void BufferReleased(
        MfxOmxPoolId id,
        OMX_BUFFERHEADERTYPE* pBuffer,
        mfxStatus error);

protected:
    MfxOmxVdecComponent(
        OMX_ERRORTYPE &error,
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);
    virtual OMX_ERRORTYPE Init(void);

    virtual OMX_ERRORTYPE InitPort(OMX_U32 nPortIndex);

    virtual void* AllocInputBuffer(size_t nSizeBytes);
    virtual void FreeInputBuffer(void* pBuffer);

    virtual OMX_ERRORTYPE Set_PortDefinition(OMX_PARAM_PORTDEFINITIONTYPE* pPortDef);

    virtual void MainThread(void);
    virtual void AsyncThread(void);
    virtual void Reset(void);

    virtual OMX_ERRORTYPE ValidateConfig(
        OMX_U32 kind,
        OMX_INDEXTYPE nIndex,
        OMX_PTR pConfig,
        MfxOmxInputConfig & config);

    // omx -> mfx (SetConfig)
    template<typename T>
    inline OMX_ERRORTYPE Validate(
        MfxOmxInputConfig& config,
        OMX_INDEXTYPE idx,
        const T* omxparams,
        OMX_U32 omxversion = OMX_VERSION);

    // mfx -> omx
    template<typename T>
    inline OMX_ERRORTYPE mfx2omx(
        OMX_INDEXTYPE idx,
        T* omxparams,
        const mfxVideoParam& mfxparams,
        OMX_U32 omxversion = OMX_VERSION);

    OMX_ERRORTYPE InPortParams_2_OutPortParams(void);
    OMX_ERRORTYPE PortsParams_2_MfxVideoParams(void);
    OMX_ERRORTYPE MfxVideoParams_2_PortsParams(void);

    bool IsStateTransitionReady(void);
    OMX_ERRORTYPE CommandStateSet(OMX_STATETYPE new_state);
    OMX_ERRORTYPE CommandPortDisable(OMX_U32 nPortIndex);
    OMX_ERRORTYPE CommandPortEnable(OMX_U32 nPortIndex);
    OMX_ERRORTYPE CommandFlush(OMX_U32 nPortIndex);

    virtual mfxStatus InitCodec(void);
    virtual mfxStatus ReinitCodec(void);

    virtual mfxStatus ResetInput(void);
    virtual mfxStatus ResetOutput(void);
    virtual mfxStatus ResetCodec(void);

    void CloseCodec(void);
    virtual bool CanDecode(void);
    virtual bool IsResolutionChanged(void);
    virtual mfxStatus DecodeFrame(void);

    virtual bool CheckBitstream(const mfxBitstream *bs)
    {
        MFX_OMX_UNUSED(bs);
        return true;
    }

    virtual mfxStatus GetCurrentHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS);
    virtual mfxU16 GetAsyncDepth(void);

#ifdef HEVC10HDR_SUPPORT
    void UpdateHdrStaticInfo();
#endif

protected:
    mfxIMPL m_Implementation;
    MFXVideoSession m_Session;
    MFXVideoDECODE* m_pDEC;
    MfxOmxMutex m_decoderMutex;
    mfxVideoParam m_MfxVideoParams;
    std::vector<mfxExtBuffer*> m_extBuffers;
    mfxExtVideoSignalInfo m_signalInfo;
    mfxExtBuffer m_adaptivePlayback;
    mfxExtDecVideoProcessing m_decVideoProc;

    mfxFrameAllocResponse m_AllocResponse;
    bool m_bLegacyAdaptivePlayback;
    mfxU16 m_nMaxFrameWidth;
    mfxU16 m_nMaxFrameHeight;
    mfxFrameInfo m_Crops;
    bool m_bChangeOutputPortSettings;
    bool m_bEosHandlingStarted;
    bool m_bEosHandlingFinished;
    bool m_bUseSystemMemory;
    bool m_bReinit;
    bool m_bFlush;
    bool m_bErrorReportingEnabled;
    bool m_bEnableVP;
    bool m_bAllocateNativeHandle;
    bool m_bEnableNativeBuffersReceived;
    bool m_bInterlaced;

    MfxInitState m_InitState;
    MfxOmxDev* m_pDevice;
    OMX_BUFFERHEADERTYPE** m_pBufferHeaders;
    mfxU32 m_nBufferHeadersSize;

    mfxBitstream* m_pBitstream;
    MfxOmxBitstream* m_pOmxBitstream;

    mfxU32 m_nSurfacesNum;
    mfxU32 m_nSurfacesNumMin;
    MfxOmxSurfacesPool* m_pSurfaces;

    MfxOmxRing<mfxSyncPoint*> m_SyncPoints;
    mfxSyncPoint* m_pFreeSyncPoint;

    mfxU32 m_nLockedSurfacesNum;
    mfxU32 m_nCountDecodedFrames;

    MfxOmxColorAspectsWrapper m_colorAspects;

#ifdef HEVC10HDR_SUPPORT
    android::HDRStaticInfo m_SeiHDRStaticInfo;
    bool m_bIsSetHDRSEI;
#endif

    // debug files
    FILE* m_dbg_decin;
    FILE* m_dbg_decin_fc;
    FILE* m_dbg_decout;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVdecComponent)
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_VDEC_COMPONENT_H__

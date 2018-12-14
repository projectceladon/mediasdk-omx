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

#ifndef __MFX_OMX_COMPONENT_H__
#define __MFX_OMX_COMPONENT_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_ports.h"
#include "mfx_omx_buffers.h"

/*------------------------------------------------------------------------------*/

enum MfxOmxComponentType
{
    MfxOmx_None,
    MfxOmx_h264vd,
    MfxOmx_h265vd,
    MfxOmx_h264ve,
    MfxOmx_mp2vd,
    MfxOmx_vc1vd,
    MfxOmx_vp8vd,
    MfxOmx_vp9vd,
    MfxOmx_vpp,
    MfxOmx_h265ve
};

/*------------------------------------------------------------------------------*/

enum MfxOmxExtensionIndex
{
    MfxOmx_IntelAndroidExtension         = OMX_IndexKhronosExtensions + 0x00F00000,
    MfxOmx_IndexGoogleMetaDataInBuffers,            // OMX.google.android.index.storeMetaDataInBuffers
    MfxOmx_IndexGoogleAllocateNativeHandle,         // OMX.google.android.index.allocateNativeHandle
    MfxOmx_IndexGoogleUseNativeBuffers,             // OMX.google.android.index.useAndroidNativeBuffer
    MfxOmx_IndexGoogleEnableNativeBuffers,          // OMX.google.android.index.enableAndroidNativeBuffers
    MfxOmx_IndexGoogleGetNativeBufferUsage,         // OMX.google.android.index.getAndroidNativeBufferUsage
    MfxOmx_IndexGooglePrepareForAdaptivePlayback,   // OMX.google.android.index.prepareForAdaptivePlayback
    MfxOmx_IndexGooglePrependSPSPPSToIDRFrames,     // OMX.google.android.index.prependSPSPPSToIDRFrames
    MfxOmx_IndexGoogleStoreANWBufferInMetaData,     // OMX.google.android.index.storeANWBufferInMetadata
    MfxOmx_IndexGoogleDescribeColorAspects,         // OMX.google.android.index.describeColorAspects
#ifdef HDR_SEI_PAYLOAD
    MfxOmx_IndexGoogleDescribeHDRStaticInfo,        // OMX.google.android.index.describeHDRStaticInfo
#endif
    MfxOmx_IndexIntelHRDParameter,                  // OMX.intel.index.hrdparameter
    MfxOmx_IndexIntelMaxPictureSize,                // OMX.intel.index.maxpicturesize
    MfxOmx_IndexIntelTargetUsage,                   // OMX.intel.index.targetusage
    MfxOmx_IndexIntelUserData,                      // OMX.intel.index.userdata
    MfxOmx_IndexIntelEncoderFrameCropping,          // OMX.intel.index.encodeframecropping
    MfxOmx_IndexIntelEncoderVUIControl,             // OMX.intel.index.encodevuicontrol
    MfxOmx_IndexIntelEncoderDirtyRect,              // OMX.intel.index.encodedirtyrect
    MfxOmx_IndexIntelEncoderDummyFrame,             // OMX.intel.index.configdummyframe
    MfxOmx_IndexIntelEncoderSliceNumber,            // OMX.intel.index.configslicenumber
    MfxOmx_IndexIntelEnableInternalSkip,            // OMX.intel.index.enableInternalSkip
    MfxOmx_IndexIntelBitrateLimitOff,               // OMX.intel.index.bitratelimitoff
    MfxOmx_IndexIntelNumRefFrame,                   // OMX.intel.index.numrefframe
    MfxOmx_IndexIntelGopPicSize,                    // OMX.intel.index.goppicsize
    MfxOmx_IndexIntelIdrInterval,                   // OMX.intel.index.idrinterval
    MfxOmx_IndexIntelGopRefDist,                    // OMX.intel.index.goprefdist
    MfxOmx_IndexIntelLowPower,                      // OMX.intel.index.lowpower
    MfxOmx_IndexIntelDecodedOrder,                  // OMX.intel.index.decodedorder
    MfxOmx_IndexIntelDisableDeblockingIdc,          // OMX.intel.index.disabledeblockingidc
    MfxOmx_IndexIntelEnableSFC,                     // OMX.intel.android.index.enableSFC
};

inline bool operator==(MfxOmxExtensionIndex left, OMX_INDEXTYPE right)
{
    return (static_cast<OMX_INDEXTYPE>(left) == right);
}

/*------------------------------------------------------------------------------*/

struct MfxOmxCommandData
{
    OMX_COMMANDTYPE m_command;
    union
    {
        OMX_STATETYPE m_new_state; /* OMX_CommandStateSet */
        OMX_U32 m_port_number; /* OMX_CommandFlush, OMX_CommandPortDisable,
                                * OMX_CommandPortEnable, OMX_CommandMarkBuffer */
    };
    OMX_MARKTYPE m_mark; /* OMX_CommandMarkBuffer */
};

/*------------------------------------------------------------------------------*/

struct MfxOmxInputData
{
    enum {
        MfxOmx_InputData_None,
        MfxOmx_InputData_Command, // OMX_SendCommand
        MfxOmx_InputData_Config,  // OMX_SetConfig
        MfxOmx_InputData_Buffer,  // OMX_EmptyThisBuffer
    } type;
    union {
        MfxOmxCommandData command;
        MfxOmxInputConfig config;
        OMX_BUFFERHEADERTYPE* buffer;
    };
};

/*------------------------------------------------------------------------------*/

struct MfxOmxComponentRegData;
class MfxOmxComponent;

typedef MfxOmxComponent* MfxOmxComponentCreateFunc(
    OMX_HANDLETYPE self,
    OMX_BOOL bCreateComponent,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags);

/*------------------------------------------------------------------------------*/

struct MfxOmxComponentRegData
{
    char* m_name;
    MfxOmxComponentType m_type;
    MfxOmxComponentCreateFunc* m_create_func;
    OMX_U32 m_ports_num;
    MfxOmxPortRegData** m_ports;
    OMX_U32 m_roles_num;
    char** m_roles;
};

/*------------------------------------------------------------------------------*/

class MfxOmxComponent
{
    friend unsigned int MfxOmxComponent_MainThread(void* param);
    friend unsigned int MfxOmxComponent_AsyncThread(void* param);

public:
    virtual ~MfxOmxComponent(void);

    enum
    {
        eSetParameter,
        eSetConfig
    };

    // MfxOmxComponent methods
    virtual OMX_ERRORTYPE GetComponentVersion(
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID);
    virtual OMX_ERRORTYPE SendCommand(
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam,
        OMX_IN  OMX_PTR pCmdData);
    virtual OMX_ERRORTYPE GetParameter(
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);
    virtual OMX_ERRORTYPE SetParameter(
        OMX_IN OMX_INDEXTYPE nParamIndex,
        OMX_IN OMX_PTR pComponentParameterStructure);
    virtual OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);
    virtual OMX_ERRORTYPE SetConfig(
        OMX_IN OMX_INDEXTYPE nIndex,
        OMX_IN OMX_PTR pComponentConfigStructure);
    virtual OMX_ERRORTYPE GetState(
        OMX_OUT OMX_STATETYPE* pState);
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
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    virtual OMX_ERRORTYPE FillThisBuffer(
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    virtual OMX_ERRORTYPE SetCallbacks(
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);
    virtual OMX_ERRORTYPE UseEGLImage(
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN void* eglImage);
    virtual OMX_ERRORTYPE ComponentRoleEnum(
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);

protected:
    MfxOmxComponent(
            OMX_HANDLETYPE self,
            OMX_U32 flags);

    MfxOmxComponent(
        OMX_ERRORTYPE &error,
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);
    virtual OMX_ERRORTYPE Init(void);

    virtual OMX_ERRORTYPE InitPort(OMX_U32 nPortIndex);
    virtual OMX_ERRORTYPE Set_PortDefinition(OMX_PARAM_PORTDEFINITIONTYPE* pPortDef);
    virtual OMX_ERRORTYPE Set_VideoPortFormat(OMX_VIDEO_PARAM_PORTFORMATTYPE* pVideoFormat);

    virtual void MainThread(void) = 0;
    virtual void AsyncThread(void) = 0;
    virtual OMX_ERRORTYPE InternalThreadsWait(void);
    virtual OMX_ERRORTYPE ValidateCommand(MfxOmxCommandData *command);
    virtual OMX_ERRORTYPE ValidateConfig(
        OMX_U32 kind,
        OMX_INDEXTYPE nIndex,
        OMX_PTR pComponentConfigStructure,
        MfxOmxInputConfig & config) = 0;

    virtual mfxU16 GetAsyncDepth(void) = 0;

protected: // inlines
    inline bool IsPortValid(OMX_U32 nPortIndex)
    {
        if (nPortIndex >= m_pRegData->m_ports_num) return false;
        return true;
    }

    inline bool IsIndexValid(OMX_INDEXTYPE nIndex, OMX_U32 nPortIndex)
    {
        if (!IsPortValid(nPortIndex)) return false;
        return mfx_omx_is_index_valid(nIndex, m_pRegData->m_ports[nPortIndex]->m_port_id);
    }

    inline bool IsStateValidForProcessing(void)
    {
        bool isValidState;

        isValidState = (OMX_StateIdle != m_state) &&
                       (OMX_StateExecuting != m_state) &&
                       (OMX_StatePause != m_state);

        /* Is we are in transition Executing->Idle or Idle->Load we should not
         * accept new frames for processing. At this moment the component is trying
         * to release all buffers it has.
         */
        isValidState |= (OMX_StateExecuting == m_state && OMX_StateIdle == m_state_to_set)
                     || (OMX_StateIdle == m_state && OMX_StateLoaded == m_state_to_set);

        return isValidState;
    }

    inline bool IsPortEnabled(OMX_U32 nPortIndex)
    {
        return (OMX_TRUE == m_pPorts[nPortIndex]->m_port_def.bEnabled);
    }

    inline bool ArePortsEnabled(OMX_U32 nPortIndex)
    {
        if (OMX_ALL == nPortIndex)
        { // OMX_ALL case
            OMX_U32 i = 0;

            for (i = 0; i < m_pRegData->m_ports_num; ++i)
            {
                if (!IsPortEnabled(i)) return false;
            }
            return true;
        }
        // single port case is here
        return IsPortEnabled(nPortIndex);
    }

protected: // variables
    OMX_HANDLETYPE m_self;
    OMX_CALLBACKTYPE* m_pCallbacks;
    OMX_PTR m_pAppData;

    MfxOmxComponentRegData* m_pRegData;
    MfxOmxPortData** m_pPorts;
    mfxStatus m_Error;
    OMX_U32 m_Flags;

    OMX_PARAM_PORTDEFINITIONTYPE* m_pInPortDef;
    OMX_PARAM_PORTDEFINITIONTYPE* m_pOutPortDef;
    MFX_OMX_PARAM_PORTINFOTYPE* m_pInPortInfo;
    MFX_OMX_PARAM_PORTINFOTYPE* m_pOutPortInfo;

    // current state of the component
    OMX_STATETYPE m_state;
    // state of the component to transition to
    OMX_STATETYPE m_state_to_set;

    MfxOmxMutex m_mutex;

    bool m_bDestroy;
    bool m_bTransition;
    bool m_bOnFlySurfacesAllocation;
    bool m_bANWBufferInMetaData;
    // input (commands, data and configs) ring buffer
    MfxOmxRing<MfxOmxInputData> m_input_queue;
    // main component thread
    MfxOmxThread* m_pMainThread;
    // async encoder thread component
    MfxOmxThread* m_pAsyncThread;
    // semaphore which notifies main sent new sync point to wait
    MfxOmxSemaphore* m_pAsyncSemaphore;
    // event notifies the MainThread that all buffered frames are obtained
    MfxOmxEvent* m_pAllSyncOpFinished;
    // semaphore which notifies main thread than new command is available for processing
    MfxOmxSemaphore* m_pCommandsSemaphore;
    // notifies component threads that conditions for requiested state change were satisfied
    MfxOmxEvent* m_pStateTransitionEvent;
    // helps to handle MFX_WRN_DEVICE_BUSY
    MfxOmxEvent* m_pDevBusyEvent;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxComponent)
};

/*------------------------------------------------------------------------------*/

class MfxOmxDummyComponent : public MfxOmxComponent
{
public:
    static MfxOmxComponent* Create(
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);

    virtual ~MfxOmxDummyComponent(void);

protected:
    MfxOmxDummyComponent(
        OMX_ERRORTYPE &error,
        OMX_HANDLETYPE self,
        MfxOmxComponentRegData* reg_data,
        OMX_U32 flags);

    virtual void MainThread(void) {}
    virtual void AsyncThread(void) {}
    virtual OMX_ERRORTYPE ValidateConfig(
        OMX_U32 kind,
        OMX_INDEXTYPE nIndex,
        OMX_PTR pComponentConfigStructure,
        MfxOmxInputConfig & config)
    {
        MFX_OMX_UNUSED(kind);
        MFX_OMX_UNUSED(nIndex);
        MFX_OMX_UNUSED(pComponentConfigStructure);
        MFX_OMX_UNUSED(config);

        return OMX_ErrorNone;
    }
    virtual mfxU16 GetAsyncDepth(void) { return 0;}

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxDummyComponent)
};

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern MfxOmxComponent* mfx_omx_create_component(
    OMX_HANDLETYPE self,
    OMX_BOOL bCreateComponent,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags);

// OMX Component functions
extern OMX_ERRORTYPE MFX_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
    OMX_OUT OMX_UUIDTYPE* pComponentUUID);

extern OMX_ERRORTYPE MFX_OMX_SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData);

extern OMX_ERRORTYPE MFX_OMX_GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure);

extern OMX_ERRORTYPE MFX_OMX_SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure);

extern OMX_ERRORTYPE MFX_OMX_GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure);

extern OMX_ERRORTYPE MFX_OMX_SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure);

extern OMX_ERRORTYPE MFX_OMX_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType);

extern OMX_ERRORTYPE MFX_OMX_GetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState);

extern OMX_ERRORTYPE MFX_OMX_ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComp,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);

extern OMX_ERRORTYPE MFX_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer);

extern OMX_ERRORTYPE MFX_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes);

extern OMX_ERRORTYPE MFX_OMX_FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

extern OMX_ERRORTYPE MFX_OMX_EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

extern OMX_ERRORTYPE MFX_OMX_FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

extern OMX_ERRORTYPE MFX_OMX_SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData);

extern OMX_ERRORTYPE MFX_OMX_ComponentDeInit(
    OMX_IN  OMX_HANDLETYPE hComponent);

extern OMX_ERRORTYPE MFX_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN void* eglImage);

extern OMX_ERRORTYPE MFX_OMX_ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_COMPONENT_H__

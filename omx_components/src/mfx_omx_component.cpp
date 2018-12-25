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

#include "mfx_omx_utils.h"
#include "mfx_omx_component.h"
#include "mfx_omx_vdec_component.h"
#include "mfx_omx_venc_component.h"
#include <cutils/properties.h>

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_component"

/*------------------------------------------------------------------------------*/

// default buffer count on port
#define MFX_BUFFER_COUNT_MIN      1
#define MFX_BUFFER_COUNT_ACTUAL   4

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

MfxOmxComponent* mfx_omx_create_component(
    OMX_HANDLETYPE self,
    OMX_BOOL bCreateComponent,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxComponent* pComponent = NULL;
    MFX_OMX_AUTO_TRACE_P(self);
    MFX_OMX_AUTO_TRACE_P(reg_data);
    MFX_OMX_AUTO_TRACE_U32(flags);

    if (bCreateComponent)
    {
        if (reg_data)
        {
            MFX_OMX_AUTO_TRACE_S(reg_data->m_name);
            MFX_OMX_AUTO_TRACE_I32(reg_data->m_type);

            if ((MfxOmx_h264vd == reg_data->m_type) ||
                (MfxOmx_h265vd == reg_data->m_type) ||
                (MfxOmx_mp2vd  == reg_data->m_type) ||
                (MfxOmx_vc1vd  == reg_data->m_type) ||
                (MfxOmx_vp8vd  == reg_data->m_type) ||
                (MfxOmx_vp9vd  == reg_data->m_type))
            {
                pComponent = MfxOmxVdecComponent::Create(self, reg_data, flags);
            }
            else if ((MfxOmx_h264ve == reg_data->m_type)
                  || (MfxOmx_h265ve == reg_data->m_type))
            {
                pComponent = MfxOmxVencComponent::Create(self, reg_data, flags);
            }
        }
    }
    else
    {
        pComponent = MfxOmxDummyComponent::Create(self, reg_data, flags);
    }
    MFX_OMX_AUTO_TRACE_P(pComponent);
    return pComponent;
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

unsigned int MfxOmxComponent_MainThread(void* param)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    unsigned int res = 0;
    MfxOmxComponent* pComponent = (MfxOmxComponent*) param;

    MFX_OMX_AUTO_TRACE_P(pComponent);
    if (pComponent) pComponent->MainThread();
    else res = 1;
    return res;
}

/*------------------------------------------------------------------------------*/

unsigned int MfxOmxComponent_AsyncThread(void* param)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    unsigned int res = 0;
    MfxOmxComponent* pComponent = (MfxOmxComponent*) param;

    MFX_OMX_AUTO_TRACE_P(pComponent);
    if (pComponent) pComponent->AsyncThread();
    else res = 1;
    return res;
}

/*------------------------------------------------------------------------------*/

MfxOmxComponent::MfxOmxComponent(OMX_HANDLETYPE self,
                                 OMX_U32 flags)
    : m_self(self)
    , m_pCallbacks(NULL)
    , m_pAppData(NULL)
    , m_pRegData(NULL)
    , m_pPorts(NULL)
    , m_Error(MFX_ERR_NONE)
    , m_Flags(flags)
    , m_pInPortDef(NULL)
    , m_pOutPortDef(NULL)
    , m_pInPortInfo(NULL)
    , m_pOutPortInfo(NULL)
    , m_state(OMX_StateLoaded)
    , m_state_to_set(OMX_StateLoaded)
    , m_bDestroy(false)
    , m_bTransition(false)
    , m_bOnFlySurfacesAllocation(false)
    , m_bANWBufferInMetaData(false)
    , m_pMainThread(NULL)
    , m_pAsyncThread(NULL)
    , m_pAsyncSemaphore(NULL)
    , m_pAllSyncOpFinished(NULL)
    , m_pCommandsSemaphore(NULL)
    , m_pStateTransitionEvent(NULL)
    , m_pDevBusyEvent(NULL)
{
}

/*------------------------------------------------------------------------------*/

MfxOmxComponent::MfxOmxComponent(OMX_ERRORTYPE &error,
                                 OMX_HANDLETYPE self,
                                 MfxOmxComponentRegData* reg_data,
                                 OMX_U32 flags)
    : MfxOmxComponent(self, flags)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_P(reg_data);

    error = OMX_ErrorNone;
    if (OMX_ErrorNone == error)
    {
        if (!reg_data || !(reg_data->m_name))
        {
            error = OMX_ErrorBadParameter;
        }
        else
        {
            m_pRegData = reg_data;

            MFX_OMX_AUTO_TRACE_S(m_pRegData->m_name);
        }
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pCommandsSemaphore, MfxOmxSemaphore());
        if (!m_pCommandsSemaphore) error = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pAsyncSemaphore, MfxOmxSemaphore());
        if (!m_pAsyncSemaphore) error = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pAllSyncOpFinished, MfxOmxEvent(true, true));
        if (!m_pAllSyncOpFinished) error = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pStateTransitionEvent, MfxOmxEvent(false, false));
        if (!m_pStateTransitionEvent) error = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone == error)
    {
        MFX_OMX_NEW(m_pDevBusyEvent, MfxOmxEvent(false, false));
        if (!m_pDevBusyEvent) error = OMX_ErrorInsufficientResources;
    }

    g_OmxLogLevel = 0;
    char value[128];
    if (property_get("OMX.Intel.debug", value, 0))
    {
        g_OmxLogLevel = atoi(value);
    }
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "Debug logs are enabled");

    MFX_OMX_AUTO_TRACE_U32(error);
}

/*------------------------------------------------------------------------------*/

MfxOmxComponent::~MfxOmxComponent(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_I32(m_state);

    MFX_OMX_DELETE(m_pMainThread);
    MFX_OMX_DELETE(m_pCommandsSemaphore);
    MFX_OMX_DELETE(m_pStateTransitionEvent);
    MFX_OMX_DELETE(m_pDevBusyEvent);
    MFX_OMX_DELETE(m_pAsyncThread);
    MFX_OMX_DELETE(m_pAsyncSemaphore);
    MFX_OMX_DELETE(m_pAllSyncOpFinished);

    if (m_pPorts && m_pRegData)
    {
        for (mfxU32 i = 0; i < m_pRegData->m_ports_num; ++i)
        {
            MFX_OMX_FREE(m_pPorts[i]);
        }
    }
    MFX_OMX_FREE(m_pPorts);
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::InternalThreadsWait(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    m_bDestroy = true;
    MFX_OMX_AUTO_TRACE_I32(m_bTransition);
    if (m_bTransition)
    {
         MFX_OMX_AUTO_TRACE_MSG("Sending m_pStateTransitionEvent");
         m_bTransition = false;
         m_pStateTransitionEvent->Signal();
    }
    if (m_pMainThread)
    {
        if (m_pCommandsSemaphore) m_pCommandsSemaphore->Post();
        m_pMainThread->Wait();
    }
    if (m_pAsyncThread)
    {
        if (m_pAsyncSemaphore) m_pAsyncSemaphore->Post();
        m_pAsyncThread->Wait();
    }

    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::Init(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    if (OMX_ErrorNone == omx_res)
    { // creating component ports
        mfxU32 i = 0;

        m_pPorts = (MfxOmxPortData**)calloc(m_pRegData->m_ports_num, sizeof(MfxOmxPortData*));
        if (m_pPorts)
        {
            for (i = 0; (OMX_ErrorNone == omx_res) && (i < m_pRegData->m_ports_num); ++i)
            {
                m_pPorts[i] = m_pRegData->m_ports[i]->m_create_func(m_pRegData->m_ports[i]);
                if (m_pPorts[i]) omx_res = InitPort(i);
                else omx_res = OMX_ErrorUndefined;
            }
        }
        else omx_res = OMX_ErrorUndefined;
    }
    if (OMX_ErrorNone == omx_res)
    {
        m_pInPortDef = &(m_pPorts[MFX_OMX_INPUT_PORT_INDEX]->m_port_def);
        m_pOutPortDef = &(m_pPorts[MFX_OMX_OUTPUT_PORT_INDEX]->m_port_def);
        m_pInPortInfo = &(m_pPorts[MFX_OMX_INPUT_PORT_INDEX]->m_port_info);
        m_pOutPortInfo = &(m_pPorts[MFX_OMX_OUTPUT_PORT_INDEX]->m_port_info);
    }
    if (OMX_ErrorNone == omx_res)
    {
        MFX_OMX_NEW(m_pMainThread, MfxOmxThread(MfxOmxComponent_MainThread, this));
        if (!m_pMainThread) omx_res = OMX_ErrorInsufficientResources;
    }
    if (OMX_ErrorNone == omx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("Async thread creation");
        MFX_OMX_NEW(m_pAsyncThread, MfxOmxThread(MfxOmxComponent_AsyncThread, this));
        if (!m_pAsyncThread) omx_res = OMX_ErrorInsufficientResources;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::GetComponentVersion(
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
    OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(pComponentUUID);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(pComponentName);
    MFX_OMX_AUTO_TRACE_P(pComponentVersion);
    MFX_OMX_AUTO_TRACE_P(pSpecVersion);
    MFX_OMX_AUTO_TRACE_P(pComponentUUID);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && (!pComponentVersion ||
                                       !pSpecVersion /*||
                                       !pComponentUUID*/)) // pComponentUUID is optional
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && (OMX_StateInvalid == m_state))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    // returning information on the component
    if (OMX_ErrorNone == omx_res)
    {
        strcpy(pComponentName, m_pRegData->m_name);
        pComponentVersion->nVersion = MFX_OMX_VERSION;
        pSpecVersion->nVersion = OMX_VERSION;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::ValidateCommand(MfxOmxCommandData *command)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    if (!command) omx_res = OMX_ErrorBadParameter;
    if ((OMX_ErrorNone == omx_res) &&
        (OMX_CommandStateSet != command->m_command) &&
        (OMX_CommandFlush != command->m_command) &&
        (OMX_CommandPortDisable != command->m_command) &&
        (OMX_CommandPortEnable != command->m_command) &&
        (OMX_CommandMarkBuffer != command->m_command))
    {
        omx_res = OMX_ErrorBadParameter;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::SendCommand(
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32 nParam,
    OMX_IN OMX_PTR pCmdData)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_I32(Cmd);
    MFX_OMX_AUTO_TRACE_I32(nParam);
    MFX_OMX_AUTO_TRACE_P(pCmdData);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && (OMX_StateInvalid == m_state))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    if ((OMX_ErrorNone == omx_res) && (OMX_CommandMarkBuffer == Cmd) && !pCmdData)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // sending command
    if (OMX_ErrorNone == omx_res)
    {
        MfxOmxInputData input;
        MfxOmxCommandData& command = input.command;

        MFX_OMX_ZERO_MEMORY(input);
        input.type = MfxOmxInputData::MfxOmx_InputData_Command;

        command.m_command = Cmd;
        switch (Cmd)
        {
        case OMX_CommandStateSet:
            MFX_OMX_AUTO_TRACE_MSG("OMX_CommandStateSet");
            command.m_new_state = (OMX_STATETYPE)nParam;
            break;
        case OMX_CommandFlush:
            MFX_OMX_AUTO_TRACE_MSG("OMX_CommandFlush");
            command.m_port_number = nParam;
            break;
        case OMX_CommandPortDisable:
            MFX_OMX_AUTO_TRACE_MSG("OMX_CommandPortDisable");
            command.m_port_number = nParam;
            break;
        case OMX_CommandPortEnable:
            MFX_OMX_AUTO_TRACE_MSG("OMX_CommandPortEnable");
            command.m_port_number = nParam;
            break;
        case OMX_CommandMarkBuffer:
            MFX_OMX_AUTO_TRACE_MSG("OMX_CommandMarkBuffer");
            command.m_port_number = nParam;
            command.m_mark = *(reinterpret_cast<OMX_MARKTYPE*>(pCmdData));
            break;
        default:
            MFX_OMX_AUTO_TRACE_MSG("unknown command");
            break;
        }
        omx_res = ValidateCommand(&command);
        if (OMX_ErrorNone == omx_res)
        {
            if (m_input_queue.Add(&input)) m_pCommandsSemaphore->Post();
            else omx_res = OMX_ErrorInsufficientResources;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::GetParameter(
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_U32(nParamIndex);
    MFX_OMX_AUTO_TRACE_P(pComponentParameterStructure);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pComponentParameterStructure)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && (OMX_StateInvalid == m_state))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        switch (nParamIndex)
        {
        case OMX_IndexParamVideoInit:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoInit");
                OMX_PORT_PARAM_TYPE* pParam = (OMX_PORT_PARAM_TYPE*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_PORT_PARAM_TYPE>(pParam, sizeof(OMX_PORT_PARAM_TYPE), OMX_VERSION))
                {
                    pParam->nPorts = m_pRegData->m_ports_num;
                    pParam->nStartPortNumber = 0;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamVideoProfileLevelQuerySupported:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamVideoProfileLevelQuerySupported");
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* pParam = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_VIDEO_PARAM_PROFILELEVELTYPE>(pParam, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE), OMX_VERSION))
                {
                    MFX_OMX_AUTO_TRACE_I32(pParam->nPortIndex);
                    if (IsIndexValid(OMX_IndexParamVideoProfileLevelQuerySupported, pParam->nPortIndex))
                    {
                        MfxOmxVideoPortRegData* video_reg = (MfxOmxVideoPortRegData*)(m_pRegData->m_ports[pParam->nPortIndex]);
                        OMX_U32 profile_index = pParam->nProfileIndex;

                        MFX_OMX_AUTO_TRACE_I32(profile_index);
                        if (profile_index < video_reg->m_profile_levels_num)
                        {
                            pParam->eProfile = video_reg->m_profile_levels[profile_index].profile;
                            pParam->eLevel = video_reg->m_profile_levels[profile_index].level;

                            MFX_OMX_AT__OMX_VIDEO_PARAM_PROFILELEVELTYPE((*pParam));
                        }
                        else omx_res = OMX_ErrorNoMore;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
       case OMX_IndexParamStandardComponentRole:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamStandardComponentRole");
                OMX_PARAM_COMPONENTROLETYPE* pParam = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

                if (IsStructVersionValid<OMX_PARAM_COMPONENTROLETYPE>(pParam, sizeof(OMX_PARAM_COMPONENTROLETYPE), OMX_VERSION))
                {
                     omx_res = OMX_ErrorNoMore;
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
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

/* NOTE: some parameters are read-only, we should not generate error if caller
 * attempts to change them.
 */
OMX_ERRORTYPE MfxOmxComponent::SetParameter(
    OMX_IN OMX_INDEXTYPE nParamIndex,
    OMX_IN OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    bool bIncorrectState = (OMX_StateLoaded != m_state) && (OMX_StateWaitForResources != m_state);

    MFX_OMX_AUTO_TRACE_U32(nParamIndex);
    MFX_OMX_AUTO_TRACE_P(pComponentParameterStructure);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pComponentParameterStructure)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && bIncorrectState && ArePortsEnabled(OMX_ALL))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        switch (nParamIndex)
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
                        if (!bIncorrectState || !IsPortEnabled(pParam->nPortIndex))
                        {
                            omx_res = Set_PortDefinition(pParam);
                        }
                        else omx_res = OMX_ErrorInvalidState;
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
                        if (!bIncorrectState || !IsPortEnabled(pParam->nPortIndex))
                        {
                            omx_res = Set_VideoPortFormat(pParam);
                        }
                        else omx_res = OMX_ErrorInvalidState;
                    }
                    else omx_res = OMX_ErrorBadPortIndex;
                }
                else omx_res = OMX_ErrorVersionMismatch;
            }
            break;
        case OMX_IndexParamStandardComponentRole:
            {
                MFX_OMX_AUTO_TRACE_MSG("OMX_IndexParamStandardComponentRole");
                OMX_PARAM_COMPONENTROLETYPE* pParam = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;
                if (IsStructVersionValid<OMX_PARAM_COMPONENTROLETYPE>(pParam, sizeof(OMX_PARAM_COMPONENTROLETYPE), OMX_VERSION))
                {
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
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::GetConfig(
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_U32(nIndex);
    MFX_OMX_AUTO_TRACE_P(pComponentConfigStructure);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pComponentConfigStructure)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && (OMX_StateInvalid == m_state))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        switch (nIndex)
        {
        default:
            MFX_OMX_AUTO_TRACE_MSG("unknown nParamIndex");
            omx_res = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::SetConfig(
    OMX_IN OMX_INDEXTYPE nIndex,
    OMX_IN OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_U32(nIndex);
    MFX_OMX_AUTO_TRACE_P(pComponentConfigStructure);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pComponentConfigStructure)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && (OMX_StateInvalid == m_state))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    if (OMX_ErrorNone == omx_res)
    {
        MfxOmxInputData input;

        MFX_OMX_ZERO_MEMORY(input);
        input.type = MfxOmxInputData::MfxOmx_InputData_Config;

        omx_res = ValidateConfig(eSetConfig, nIndex, pComponentConfigStructure, input.config);
        if ((OMX_ErrorNone == omx_res) &&
            (input.config.mfxparams ||
             input.config.control))
        {
            if (m_input_queue.Add(&input)) m_pCommandsSemaphore->Post();
            else omx_res = OMX_ErrorInsufficientResources;
        }
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::InitPort(OMX_U32 nPortIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    SetStructVersion<OMX_PARAM_PORTDEFINITIONTYPE>(&(m_pPorts[nPortIndex]->m_port_def));
    m_pPorts[nPortIndex]->m_port_def.nPortIndex = m_pRegData->m_ports[nPortIndex]->m_port_index;
    m_pPorts[nPortIndex]->m_port_def.eDir = m_pRegData->m_ports[nPortIndex]->m_port_direction;
    m_pPorts[nPortIndex]->m_port_def.bEnabled = OMX_TRUE;
    m_pPorts[nPortIndex]->m_port_def.bPopulated = OMX_FALSE;
    if (m_pRegData->m_ports[nPortIndex]->m_port_id & MfxOmxPortVideo)
    {
        MfxOmxVideoPortRegData* video_reg = (MfxOmxVideoPortRegData*)(m_pRegData->m_ports[nPortIndex]);

        m_pPorts[nPortIndex]->m_port_def.eDomain = OMX_PortDomainVideo;
        m_pPorts[nPortIndex]->m_port_def.format.video.eCompressionFormat = video_reg->m_formats[0].eCompressionFormat;
        m_pPorts[nPortIndex]->m_port_def.format.video.eColorFormat = video_reg->m_formats[0].eColorFormat;
        m_pPorts[nPortIndex]->m_port_def.format.video.xFramerate = MFX_OMX_DEFAULT_FRAMERATE << 16;
        m_pPorts[nPortIndex]->m_port_def.format.video.nBitrate = MFX_OMX_DEFAULT_BITRATE;
        m_pPorts[nPortIndex]->m_port_def.format.video.nFrameWidth = MFX_OMX_DEFAULT_FRAME_WIDTH;
        m_pPorts[nPortIndex]->m_port_def.format.video.nFrameHeight = MFX_OMX_DEFAULT_FRAME_HEIGHT;
        m_pPorts[nPortIndex]->m_port_def.format.video.nStride = MFX_OMX_DEFAULT_FRAME_WIDTH;
        m_pPorts[nPortIndex]->m_port_def.format.video.nSliceHeight = MFX_OMX_DEFAULT_FRAME_HEIGHT;
    }
    m_pPorts[nPortIndex]->m_port_def.nBufferCountMin = MFX_BUFFER_COUNT_MIN;
    m_pPorts[nPortIndex]->m_port_def.nBufferCountActual = MFX_BUFFER_COUNT_ACTUAL;

    mfx_omx_adjust_port_definition(&(m_pPorts[nPortIndex]->m_port_def), NULL,
                                   m_bOnFlySurfacesAllocation,
                                   m_bANWBufferInMetaData);

    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE(m_pPorts[nPortIndex]->m_port_def);

    m_pPorts[nPortIndex]->m_port_info.nPortIndex = m_pRegData->m_ports[nPortIndex]->m_port_index;
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::Set_PortDefinition(
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_VIDEO_PORTDEFINITIONTYPE* pVideoFormat = NULL;
    OMX_VIDEO_PORTDEFINITIONTYPE* pPortVideoFormat = NULL;
    OMX_U32 port = 0;
    bool bColorFormat = false, bPortColorFormat = false;

    if (!pPortDef) omx_res = OMX_ErrorBadParameter;
    if (OMX_ErrorNone == omx_res)
    {
        port = pPortDef->nPortIndex;

        pVideoFormat = &(pPortDef->format.video);
        pPortVideoFormat = &(m_pPorts[port]->m_port_def.format.video);

        MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE((*pPortDef));

        bColorFormat = (OMX_VIDEO_CodingUnused == pVideoFormat->eCompressionFormat);
        bPortColorFormat = (OMX_VIDEO_CodingUnused == pPortVideoFormat->eCompressionFormat);

        if ((bColorFormat && !bPortColorFormat) || (!bColorFormat && bPortColorFormat))
        {
            omx_res = OMX_ErrorBadParameter;
        }
        if (bColorFormat)
        {
            bColorFormat = (OMX_COLOR_FormatUnused != pVideoFormat->eColorFormat);
            if (!bColorFormat) omx_res = OMX_ErrorBadParameter;
        }
    }
    if (OMX_ErrorNone == omx_res)
    {
        // setting video parameters
        *pPortVideoFormat = *pVideoFormat;

        mfx_omx_adjust_port_definition(&(m_pPorts[port]->m_port_def), NULL,
                                       m_bOnFlySurfacesAllocation,
                                       m_bANWBufferInMetaData);
    }
    if (OMX_ErrorNone == omx_res)
    {
        // setting non read-only general port parameters
        if (pPortDef->nBufferCountActual >= m_pPorts[port]->m_port_def.nBufferCountMin)
        {
            m_pPorts[port]->m_port_def.nBufferCountActual = pPortDef->nBufferCountActual;
        }

        if (pPortDef->nBufferSize >= m_pPorts[port]->m_port_def.nBufferSize)
        {
            m_pPorts[port]->m_port_def.nBufferSize = pPortDef->nBufferSize;
        }
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::Set_VideoPortFormat(
    OMX_VIDEO_PARAM_PORTFORMATTYPE* pVideoFormat)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_VIDEO_PORTDEFINITIONTYPE* pPortVideoFormat = NULL;
    bool bColorFormat = false, bPortColorFormat = false;
    OMX_U32 i = 0, port = 0;

    MFX_OMX_AUTO_TRACE_P(pVideoFormat);

    if (!pVideoFormat) omx_res = OMX_ErrorBadParameter;
    if (OMX_ErrorNone == omx_res)
    {
        port = pVideoFormat->nPortIndex;
        pPortVideoFormat = &(m_pPorts[port]->m_port_def.format.video);

        MFX_OMX_AT__OMX_VIDEO_PARAM_PORTFORMATTYPE((*pVideoFormat));

        bColorFormat = (OMX_VIDEO_CodingUnused == pVideoFormat->eCompressionFormat);
        bPortColorFormat = (OMX_VIDEO_CodingUnused == pPortVideoFormat->eCompressionFormat);

        if ((bColorFormat && !bPortColorFormat) || (!bColorFormat && bPortColorFormat))
        {
            omx_res = OMX_ErrorBadParameter;
        }
        if (bColorFormat)
        {
            bColorFormat = (OMX_COLOR_FormatUnused != pVideoFormat->eColorFormat);
            if (!bColorFormat) omx_res = OMX_ErrorBadParameter;
        }
    }
    // searching of the proposed format among supported ones
    if (OMX_ErrorNone == omx_res)
    {
        MfxOmxVideoPortRegData* video_reg = (MfxOmxVideoPortRegData*)(m_pRegData->m_ports[port]);

        bColorFormat = (OMX_VIDEO_CodingUnused == pVideoFormat->eCompressionFormat);
        for (i = 0; i < video_reg->m_formats_num; ++i)
        {
            if (bColorFormat)
            {
                if (pVideoFormat->eColorFormat == video_reg->m_formats[i].eColorFormat) break;
            }
            else
            {
                if (pVideoFormat->eCompressionFormat == video_reg->m_formats[i].eCompressionFormat) break;
            }
        }
        if (i >= video_reg->m_formats_num) omx_res = OMX_ErrorUnsupportedSetting;
    }
    // configuring port for the new format
    if (OMX_ErrorNone == omx_res)
    {
        m_pPorts[port]->m_port_def.eDomain = OMX_PortDomainVideo;
        m_pPorts[port]->m_port_def.bEnabled = OMX_TRUE;
        m_pPorts[port]->m_port_def.bPopulated = OMX_FALSE;
        m_pPorts[port]->m_port_def.format.video.eCompressionFormat = pVideoFormat->eCompressionFormat;
        m_pPorts[port]->m_port_def.format.video.eColorFormat = pVideoFormat->eColorFormat;
        m_pPorts[port]->m_port_def.format.video.xFramerate = pVideoFormat->xFramerate;
        m_pPorts[port]->m_port_def.format.video.nFrameWidth = MFX_OMX_DEFAULT_FRAME_WIDTH;
        m_pPorts[port]->m_port_def.format.video.nFrameHeight = MFX_OMX_DEFAULT_FRAME_HEIGHT;
        m_pPorts[port]->m_port_def.format.video.nStride = MFX_OMX_DEFAULT_FRAME_WIDTH;
        m_pPorts[port]->m_port_def.format.video.nSliceHeight = MFX_OMX_DEFAULT_FRAME_HEIGHT;
        m_pPorts[port]->m_port_def.nBufferCountMin = MFX_BUFFER_COUNT_MIN;
        m_pPorts[port]->m_port_def.nBufferCountActual = MFX_BUFFER_COUNT_ACTUAL;

        mfx_omx_adjust_port_definition(&(m_pPorts[port]->m_port_def), NULL,
                                       m_bOnFlySurfacesAllocation,
                                       m_bANWBufferInMetaData);

        MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE(m_pPorts[port]->m_port_def);
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::GetState(
    OMX_OUT OMX_STATETYPE* pState)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(pState);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pState)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // getting state
    if (OMX_ErrorNone == omx_res)
    {
        *pState = m_state;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::DealWithBuffer(
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(pAppPrivate);
    MFX_OMX_UNUSED(nSizeBytes);
    MFX_OMX_UNUSED(pBuffer);
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    bool bIncorrectState = (OMX_StateLoaded != m_state) && (OMX_StateWaitForResources != m_state);

    MFX_OMX_AUTO_TRACE_P(ppBufferHdr);
    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    MFX_OMX_AUTO_TRACE_P(pAppPrivate); // can be NULL
    MFX_OMX_AUTO_TRACE_I32(nSizeBytes);
    MFX_OMX_AUTO_TRACE_P(pBuffer);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !ppBufferHdr)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsPortValid(nPortIndex))
    {
        omx_res = OMX_ErrorBadPortIndex;
    }
    if ((OMX_ErrorNone == omx_res) && bIncorrectState && IsPortEnabled(nPortIndex))
    {
        omx_res = OMX_ErrorInvalidState;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::FreeBuffer(
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_I32(nPortIndex);
    MFX_OMX_AUTO_TRACE_P(pBuffer);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pBuffer)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsPortValid(nPortIndex))
    {
        omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::EmptyThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    bool bIncorrectState = IsStateValidForProcessing();

    MFX_OMX_AUTO_TRACE_P(pBuffer);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pBuffer)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && bIncorrectState)
    {
        omx_res = OMX_ErrorInvalidState;
    }
    if ((OMX_ErrorNone == omx_res) && (MFX_OMX_INPUT_PORT_INDEX != pBuffer->nInputPortIndex))
    {
        omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::FillThisBuffer(
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    bool bIncorrectState = IsStateValidForProcessing();

    MFX_OMX_AUTO_TRACE_P(pBuffer);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !pBuffer)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && bIncorrectState)
    {
        omx_res = OMX_ErrorInvalidState;
    }
    if ((OMX_ErrorNone == omx_res) && (MFX_OMX_OUTPUT_PORT_INDEX != pBuffer->nOutputPortIndex))
    {
        omx_res = OMX_ErrorBadPortIndex;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::SetCallbacks(
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(pCallbacks);
    MFX_OMX_AUTO_TRACE_P(pAppData);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && (!pCallbacks || !pAppData))
    {
        omx_res = OMX_ErrorBadParameter;
    }
    // setting callbacks information
    if (OMX_ErrorNone == omx_res)
    {
        m_pCallbacks = pCallbacks;
        m_pAppData = pAppData;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::UseEGLImage(
    OMX_INOUT OMX_BUFFERHEADERTYPE** /*ppBufferHdr*/,
    OMX_IN OMX_U32 /*nPortIndex*/,
    OMX_IN OMX_PTR /*pAppPrivate*/,
    OMX_IN void* /*eglImage*/)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNotImplemented;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MfxOmxComponent::ComponentRoleEnum(
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;

    MFX_OMX_AUTO_TRACE_P(cRole);
    MFX_OMX_AUTO_TRACE_I32(nIndex);
    MFX_OMX_AUTO_TRACE_I32(m_pRegData->m_roles_num);

    if ((NULL != cRole) && (nIndex < m_pRegData->m_roles_num))
    {
        strcpy((char*)cRole, m_pRegData->m_roles[nIndex]);
    }
    else
    {
        omx_res = OMX_ErrorBadParameter;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

MfxOmxComponent* MfxOmxDummyComponent::Create(
    OMX_HANDLETYPE self,
    MfxOmxComponentRegData* reg_data,
    OMX_U32 flags)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    MfxOmxDummyComponent* pComponent = NULL;

    MFX_OMX_NEW(pComponent, MfxOmxDummyComponent(omx_res, self, reg_data, flags));
    if (OMX_ErrorNone != omx_res)
    {
        MFX_OMX_DELETE(pComponent);
    }
    MFX_OMX_AUTO_TRACE_P(pComponent);
    return pComponent;
}

/*------------------------------------------------------------------------------*/

MfxOmxDummyComponent::MfxOmxDummyComponent(OMX_ERRORTYPE &error,
                                           OMX_HANDLETYPE self,
                                           MfxOmxComponentRegData* reg_data,
                                           OMX_U32 flags)
    : MfxOmxComponent(self, flags)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AUTO_TRACE_P(reg_data);

    error = OMX_ErrorNone;
    if (OMX_ErrorNone == error)
    {
        if (!reg_data || !(reg_data->m_name))
        {
            error = OMX_ErrorBadParameter;
        }
        else
        {
            m_pRegData = reg_data;

            MFX_OMX_AUTO_TRACE_S(m_pRegData->m_name);
        }
    }

    MFX_OMX_AUTO_TRACE_U32(error);
}

/*------------------------------------------------------------------------------*/

MfxOmxDummyComponent::~MfxOmxDummyComponent(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

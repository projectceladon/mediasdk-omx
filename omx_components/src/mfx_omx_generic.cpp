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
#include "mfx_omx_generic.h"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

// NOTE: function should return the same error values as OMX_GetHandle.
OMX_ERRORTYPE MFX_OMX_ComponentInitFromTable(
    OMX_IN OMX_U32 nRegTableSize,
    OMX_IN MfxOmxComponentRegData* pRegTable,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_U32 nComponentFlags,
    OMX_IN OMX_BOOL bCreateComponent,
    OMX_INOUT OMX_HANDLETYPE hComponent)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;
    mfxU32 component_index = 0;

    MFX_OMX_AUTO_TRACE_I32(nRegTableSize);
    MFX_OMX_AUTO_TRACE_P(pRegTable);
    MFX_OMX_AUTO_TRACE_S(cComponentName);
    MFX_OMX_AUTO_TRACE_U32(nComponentFlags);
    MFX_OMX_AUTO_TRACE_U32(bCreateComponent);
    MFX_OMX_AUTO_TRACE_P(hComponent);
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    if ((OMX_ErrorNone == omx_res) &&
        !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    if (OMX_ErrorNone == omx_res)
    {
        for (component_index = 0; component_index < nRegTableSize; ++component_index)
        {
            MFX_OMX_AUTO_TRACE_I32(component_index);
            MFX_OMX_AUTO_TRACE_S(pRegTable[component_index].m_name);
            MFX_OMX_AUTO_TRACE_S(cComponentName);
            if (!strcmp(pRegTable[component_index].m_name, cComponentName)) break;
        }
        if (component_index >= nRegTableSize) omx_res = OMX_ErrorComponentNotFound;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = pRegTable[component_index].m_create_func(hComponent, bCreateComponent, &(pRegTable[component_index]), nComponentFlags);
        if (!pComponent) omx_res = OMX_ErrorInsufficientResources;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    if (OMX_ErrorNone == omx_res)
    {
        omx_component->pComponentPrivate = pComponent;

        omx_component->GetComponentVersion = MFX_OMX_GetComponentVersion;
        omx_component->SendCommand = MFX_OMX_SendCommand;
        omx_component->GetParameter = MFX_OMX_GetParameter;
        omx_component->SetParameter = MFX_OMX_SetParameter;
        omx_component->GetConfig = MFX_OMX_GetConfig;
        omx_component->SetConfig = MFX_OMX_SetConfig;
        omx_component->GetExtensionIndex = MFX_OMX_GetExtensionIndex;
        omx_component->GetState = MFX_OMX_GetState;
        omx_component->ComponentTunnelRequest = MFX_OMX_ComponentTunnelRequest;
        omx_component->UseBuffer = MFX_OMX_UseBuffer;
        omx_component->AllocateBuffer = MFX_OMX_AllocateBuffer;
        omx_component->FreeBuffer = MFX_OMX_FreeBuffer;
        omx_component->EmptyThisBuffer = MFX_OMX_EmptyThisBuffer;
        omx_component->FillThisBuffer = MFX_OMX_FillThisBuffer;
        omx_component->SetCallbacks = MFX_OMX_SetCallbacks;
        omx_component->ComponentDeInit = MFX_OMX_ComponentDeInit;
        omx_component->UseEGLImage = MFX_OMX_UseEGLImage;
        omx_component->ComponentRoleEnum = MFX_OMX_ComponentRoleEnum;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

// Copyright (c) 2012-2018 Intel Corporation
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

/********************************************************************************

File: mfx_omx_component.cpp

Defined functions:
  - MFX_OMX_GetComponentVersion
  - MFX_OMX_SendCommand
  - MFX_OMX_GetParameter
  - MFX_OMX_SetParameter
  - MFX_OMX_GetConfig
  - MFX_OMX_SetConfig
  - MFX_OMX_GetExtensionIndex
  - MFX_OMX_GetState
  - MFX_OMX_ComponentTunnelRequest
  - MFX_OMX_UseBuffer
  - MFX_OMX_AllocateBuffer
  - MFX_OMX_FreeBuffer
  - MFX_OMX_EmptyThisBuffer
  - MFX_OMX_FillThisBuffer
  - MFX_OMX_SetCallbacks
  - MFX_OMX_ComponentDeInit
  - MFX_OMX_UseEGLImage
  - MFX_OMX_ComponentRoleEnum

*********************************************************************************/

#include "mfx_omx_utils.h"
#include "mfx_omx_component.h"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_base_component"

/*------------------------------------------------------------------------------*/

typedef struct
{
    OMX_STRING    ParamString;
    OMX_INDEXTYPE Index;
    OMX_ERRORTYPE Ret;
} ext_index;

const ext_index gExtIndexParams[] = {
    {
      (char*)"OMX.google.android.index.enableAndroidNativeBuffers",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleEnableNativeBuffers),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.allocateNativeHandle",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleAllocateNativeHandle),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.getAndroidNativeBufferUsage",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleGetNativeBufferUsage),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.storeMetaDataInBuffers",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleMetaDataInBuffers),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.useAndroidNativeBuffer2",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleUseNativeBuffers),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.useAndroidNativeBuffer",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleUseNativeBuffers),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.prepareForAdaptivePlayback",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGooglePrepareForAdaptivePlayback),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.prependSPSPPSToIDRFrames",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGooglePrependSPSPPSToIDRFrames),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.storeANWBufferInMetadata",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleStoreANWBufferInMetaData),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.google.android.index.describeColorAspects",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleDescribeColorAspects),
      OMX_ErrorNone
    },
#ifdef HDR_SEI_PAYLOAD
    {
      (char*)"OMX.google.android.index.describeHDRStaticInfo",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexGoogleDescribeHDRStaticInfo),
      OMX_ErrorNone
    },
#endif
    {
      (char*)"OMX.intel.index.hrdparameter",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelHRDParameter),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.maxpicturesize",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelMaxPictureSize),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.targetusage",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelTargetUsage),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.userdata",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelUserData),
      OMX_ErrorNone
    },
     {
      (char*)"OMX.intel.index.userdata",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelUserData),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.encodeframecropping",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEncoderFrameCropping),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.encodevuicontrol",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEncoderVUIControl),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.encodedirtyrect",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEncoderDirtyRect),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.configdummyframe",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEncoderDummyFrame),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.configslicenumber",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEncoderSliceNumber),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.enableInternalSkip",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEnableInternalSkip),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.bitratelimitoff",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelBitrateLimitOff),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.numrefframe",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelNumRefFrame),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.goppicsize",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelGopPicSize),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.idrinterval",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelIdrInterval),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.goprefdist",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelGopRefDist),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.lowpower",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelLowPower),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.decodedorder",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelDecodedOrder),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.index.disabledeblockingidc",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelDisableDeblockingIdc),
      OMX_ErrorNone
    },
    {
      (char*)"OMX.intel.android.index.enableSFC",
      static_cast<OMX_INDEXTYPE>(MfxOmx_IndexIntelEnableSFC),
      OMX_ErrorNone
    }
};

/*------------------------------------------------------------------------------*/

inline bool IsVersionValid(OMX_VERSIONTYPE version)
{
    // TODO: implement more sophisticated version check
    return (version.nVersion == OMX_VERSION);
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
    OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // returning information on the component
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;

        MFX_OMX_AUTO_TRACE_P(pComponent);
        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->GetComponentVersion(
                  pComponentName,
                  pComponentVersion,
                  pSpecVersion,
                  pComponentUUID),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_SendCommand(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_COMMANDTYPE Cmd,
    OMX_IN OMX_U32 nParam,
    OMX_IN OMX_PTR pCmdData)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // sending command
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->SendCommand(Cmd, nParam, pCmdData),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->GetParameter(nParamIndex, pComponentParameterStructure),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->SetParameter(nParamIndex, pComponentParameterStructure),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->GetConfig(nIndex, pComponentConfigStructure),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->SetConfig(nIndex, pComponentConfigStructure),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;

    MFX_OMX_AUTO_TRACE_P(omx_component);

    if (!omx_component || !pIndexType)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if (OMX_ErrorNone == omx_res)
    {
        bool bIsFound = false;
        for (mfxU32 i = 0; i < MFX_OMX_GET_ARRAY_SIZE(gExtIndexParams); ++i)
        {
            if (!strncmp(cParameterName, gExtIndexParams[i].ParamString, strnlen_s(gExtIndexParams[i].ParamString, OMX_MAX_STRINGNAME_SIZE)))
            {
                *pIndexType = gExtIndexParams[i].Index;
                omx_res = gExtIndexParams[i].Ret;
                bIsFound = true;
                break;
            }
        }
        if (!bIsFound) omx_res = OMX_ErrorUnsupportedIndex;
    }
    MFX_OMX_AUTO_TRACE_MSG(cParameterName);
    MFX_OMX_AUTO_TRACE_U32(*pIndexType);
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_GetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // getting state
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->GetState(pState),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComp,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(hComponent);
    MFX_OMX_UNUSED(nPort);
    MFX_OMX_UNUSED(hTunneledComp);
    MFX_OMX_UNUSED(nTunneledPort);
    MFX_OMX_UNUSED(pTunnelSetup);
    OMX_ERRORTYPE omx_res = OMX_ErrorNotImplemented;

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && (!omx_component || !pBuffer))
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // using buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->DealWithBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, pBuffer),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // using buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->DealWithBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes, NULL),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // using buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->FreeBuffer(nPortIndex, pBuffer),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // emptying buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->EmptyThisBuffer(pBuffer),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // emptying buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->FillThisBuffer(pBuffer),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && (!omx_component))
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // setting callbacks information
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->SetCallbacks(pCallbacks, pAppData),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_ComponentDeInit(
    OMX_IN  OMX_HANDLETYPE hComponent)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // component deinitialization
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_DELETE(pComponent);
        }
        //else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_UseEGLImage(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN void* eglImage)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // using EGL buffer
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->UseEGLImage(ppBufferHdr, nPortIndex, pAppPrivate, eglImage),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }

    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

OMX_ERRORTYPE MFX_OMX_ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_ERRORTYPE omx_res = OMX_ErrorNone;
    OMX_COMPONENTTYPE* omx_component = (OMX_COMPONENTTYPE*)hComponent;
    MfxOmxComponent* pComponent = NULL;

    MFX_OMX_AUTO_TRACE_P(omx_component);
    // errors checking
    if ((OMX_ErrorNone == omx_res) && !omx_component)
    {
        omx_res = OMX_ErrorBadParameter;
    }
    if ((OMX_ErrorNone == omx_res) && !IsStructVersionValid<OMX_COMPONENTTYPE>(omx_component, sizeof(OMX_COMPONENTTYPE), OMX_VERSION))
    {
        omx_res = OMX_ErrorVersionMismatch;
    }
    // filling the requested structure
    if (OMX_ErrorNone == omx_res)
    {
        pComponent = (MfxOmxComponent*)omx_component->pComponentPrivate;
        MFX_OMX_AUTO_TRACE_P(pComponent);

        if (pComponent)
        {
            MFX_OMX_TRY_AND_CATCH(
              omx_res = pComponent->ComponentRoleEnum(cRole, nIndex),
              omx_res = OMX_ErrorUndefined);
        }
        else omx_res = OMX_ErrorInvalidComponent;
    }
    MFX_OMX_AUTO_TRACE_U32(omx_res);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

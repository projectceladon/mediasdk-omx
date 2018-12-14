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

#ifndef __MFX_OMX_GENERIC_H__
#define __MFX_OMX_GENERIC_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_component.h"

/*------------------------------------------------------------------------------*/

/**
 * Declares instance of OMX_VIDEO_PARAM_PORTFORMATTYPE structure intended to be
 * used as declaration of specified bitstream compression.
 */
#define MFX_OMX_DECLARE_BST_FORMAT(_port_index, _compression) \
    { \
        MFX_OMX_SET_STRUCT_VERSION(OMX_VIDEO_PARAM_PORTFORMATTYPE), \
        _port_index, /* nPortIndex */ \
        0, /* nIndex */ \
        _compression, /* eCompressionFormat */ \
        OMX_COLOR_FormatUnused, /* eColorFormat */ \
        0 /* xFramerate */ \
    }

/**
 * Declares instance of OMX_VIDEO_PARAM_PORTFORMATTYPE structure intended to be
 * used as declaration of specified color format.
 */
#define MFX_OMX_DECLARE_SRF_FORMAT(_port_index, _color_format) \
    { \
        MFX_OMX_SET_STRUCT_VERSION(OMX_VIDEO_PARAM_PORTFORMATTYPE), \
        _port_index, /* nPortIndex */ \
        0, /* nIndex */ \
        OMX_VIDEO_CodingUnused, /* eCompressionFormat */ \
        _color_format, /* eColorFormat */ \
        0 /* xFramerate */ \
    }

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECLARE_BST_INPUT_FORMAT(_compression) \
    MFX_OMX_DECLARE_BST_FORMAT(MFX_OMX_INPUT_PORT_INDEX, _compression)

#define MFX_OMX_DECLARE_BST_OUTPUT_FORMAT(_compression) \
    MFX_OMX_DECLARE_BST_FORMAT(MFX_OMX_OUTPUT_PORT_INDEX, _compression)

#define MFX_OMX_DECLARE_SRF_INPUT_FORMAT(_color_format) \
    MFX_OMX_DECLARE_SRF_FORMAT(MFX_OMX_INPUT_PORT_INDEX, _color_format)

#define MFX_OMX_DECLARE_SRF_OUTPUT_FORMAT(_color_format) \
    MFX_OMX_DECLARE_SRF_FORMAT(MFX_OMX_OUTPUT_PORT_INDEX, _color_format)

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECLARE_DEC_INPUT_PORT(_dec) \
    static const MfxOmxVideoPortRegData g_##_dec##_input_port = \
    { \
        { \
            MfxOmxPortVideo_##_dec, \
            MFX_OMX_INPUT_PORT_INDEX, \
            OMX_DirInput, \
            mfx_omx_create_##_dec##_ports \
        }, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_dec##_input_formats), \
        (OMX_VIDEO_PARAM_PORTFORMATTYPE*)g_##_dec##_input_formats, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_dec##_profile_levels), \
        (MfxOmxProfileLevelTable*)g_##_dec##_profile_levels \
    };

#define MFX_OMX_DECLARE_DEC_OUTPUT_PORT(_dec) \
    static const MfxOmxVideoPortRegData g_##_dec##_output_port = \
    { \
        { \
            MfxOmxPortVideo_##_dec, \
            MFX_OMX_OUTPUT_PORT_INDEX, \
            OMX_DirOutput, \
            mfx_omx_create_##_dec##_ports \
        }, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_dec##_output_formats), \
        (OMX_VIDEO_PARAM_PORTFORMATTYPE*)g_##_dec##_output_formats, \
        0, \
        NULL \
    };

/*------------------------------------------------------------------------------*/

/**
 * Declares input & output ports for decoder. Caller should declare decoder input
 * ports as an array of OMX_VIDEO_PARAM_PORTFORMATTYPE structures with the name
 * g_<dec>_InPortFormat prior to using of this macro.
 */
#define MFX_OMX_DECLARE_DEC_PORTS(_dec) \
    MFX_OMX_DECLARE_DEC_INPUT_PORT(_dec); \
    MFX_OMX_DECLARE_DEC_OUTPUT_PORT(_dec); \
    static const MfxOmxPortRegData* g_##_dec##_ports[] = \
    { \
        (MfxOmxPortRegData*)&g_##_dec##_input_port, \
        (MfxOmxPortRegData*)&g_##_dec##_output_port \
    };

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECLARE_ENC_INPUT_PORT(_enc) \
    static const MfxOmxVideoPortRegData g_##_enc##_input_port = \
    { \
        { \
            MfxOmxPortVideo_##_enc, \
            MFX_OMX_INPUT_PORT_INDEX, \
            OMX_DirInput, \
            mfx_omx_create_##_enc##_ports \
        }, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_enc##_input_formats), \
        (OMX_VIDEO_PARAM_PORTFORMATTYPE*)g_##_enc##_input_formats, \
        0, \
        NULL \
    };

#define MFX_OMX_DECLARE_ENC_OUTPUT_PORT(_enc) \
    static const MfxOmxVideoPortRegData g_##_enc##_output_port = \
    { \
        { \
            MfxOmxPortVideo_##_enc, \
            MFX_OMX_OUTPUT_PORT_INDEX, \
            OMX_DirOutput, \
            mfx_omx_create_##_enc##_ports \
        }, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_enc##_output_formats), \
        (OMX_VIDEO_PARAM_PORTFORMATTYPE*)g_##_enc##_output_formats, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_enc##_profile_levels), \
        (MfxOmxProfileLevelTable*)g_##_enc##_profile_levels \
    };

/*------------------------------------------------------------------------------*/

/**
 * Declares input & output ports for encoder. Caller should declare encoder input
 * ports as an array of OMX_VIDEO_PARAM_PORTFORMATTYPE structures with the name
 * g_<enc>_InPortFormat prior to using of that macro.
 */
#define MFX_OMX_DECLARE_ENC_PORTS(_enc) \
    MFX_OMX_DECLARE_ENC_INPUT_PORT(_enc); \
    MFX_OMX_DECLARE_ENC_OUTPUT_PORT(_enc); \
    static const MfxOmxPortRegData* g_##_enc##_ports[] = \
    { \
        (MfxOmxPortRegData*)&g_##_enc##_input_port, \
        (MfxOmxPortRegData*)&g_##_enc##_output_port \
    };

/*------------------------------------------------------------------------------*/

/**
 * Declares instanse of MfxOmxPortRegData structure for specified component.
 * Caller should declare component ports as an array of MfxOmxPortRegData
 * structures with the name g_<component>_Ports.
 */
#define MFX_OMX_DECLARE_COMPONENT(_name, _component) \
    { \
        (char*)_name, \
        MfxOmx_##_component, \
        mfx_omx_create_component, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_component##_ports), \
        (MfxOmxPortRegData**)g_##_component##_ports, \
        MFX_OMX_GET_ARRAY_SIZE(g_##_component##_roles), \
        (char**)g_##_component##_roles \
    }

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECLARE_COMPONENT_VERSION() \
    const char* g_MfxOmxProductName = "mediasdk_omx_product_name: " MFX_OMX_PRODUCT_NAME; \
    const char* g_MfxOmxCopyright = "mediasdk_omx_copyright: " MFX_OMX_COPYRIGHT; \
    const char* g_MfxOmxFileVersion = "mediasdk_omx_file_version: " MFX_FILE_VERSION; \
    const char* g_MfxOmxProductVersion = "mediasdk_omx_product_version: " MFX_PRODUCT_VERSION;

/*------------------------------------------------------------------------------*/

#define MFX_OMX_DECLARE_COMPONENT_ENTRY_POINT() \
    /* NOTE: function should return the same error values as OMX_GetHandle. */ \
    OMX_API OMX_ERRORTYPE MFX_OMX_ComponentInit( \
        OMX_IN OMX_STRING cComponentName, \
        OMX_IN OMX_U32 nComponentFlags, \
        OMX_IN OMX_BOOL bCreateComponent, \
        OMX_INOUT OMX_HANDLETYPE hComponent) \
    { \
        return MFX_OMX_ComponentInitFromTable( \
            MFX_OMX_GET_ARRAY_SIZE(g_mfx_omx_components), \
            g_mfx_omx_components, \
            cComponentName, \
            nComponentFlags, \
            bCreateComponent, \
            hComponent); \
    }

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern OMX_ERRORTYPE MFX_OMX_ComponentInitFromTable(
    OMX_IN OMX_U32 nRegTableSize,
    OMX_IN MfxOmxComponentRegData* pRegTable,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_U32 nComponentFlags,
    OMX_IN OMX_BOOL bCreateComponent,
    OMX_INOUT OMX_HANDLETYPE hComponent);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_GENERIC_H__

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

#ifndef __MFX_OMX_PORTS_H__
#define __MFX_OMX_PORTS_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_buffers.h"
#include "mfx_omx_structures.h"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_INPUT_PORT_INDEX 0
#define MFX_OMX_OUTPUT_PORT_INDEX 1

#define MFX_OMX_DEFAULT_PORTS_NUMBER 2

#define MFX_OMX_DEFAULT_FRAME_WIDTH 640
#define MFX_OMX_DEFAULT_FRAME_HEIGHT 480
#define MFX_OMX_DEFAULT_BITRATE 1100000
#define MFX_OMX_DEFAULT_FRAMERATE 30U


/*------------------------------------------------------------------------------*/

enum MfxOmxPortId
{
    MfxOmxPortVideo = 0x1000,
    MfxOmxPortVideo_raw = MfxOmxPortVideo+1,
    MfxOmxPortVideo_h264vd = MfxOmxPortVideo+2,
    MfxOmxPortVideo_h265vd = MfxOmxPortVideo+3,
    MfxOmxPortVideo_h264ve = MfxOmxPortVideo+4,
    MfxOmxPortVideo_mp2vd = MfxOmxPortVideo+5,
    MfxOmxPortVideo_vc1vd = MfxOmxPortVideo+6,
    MfxOmxPortVideo_vp8vd = MfxOmxPortVideo+9,
    MfxOmxPortVideo_vp9vd = MfxOmxPortVideo+10,
    MfxOmxPortVideo_h265ve = MfxOmxPortVideo+12
};

/*------------------------------------------------------------------------------*/

struct MfxOmxPortData
{
    MfxOmxPortId m_port_id;
    MFX_OMX_PARAM_PORTINFOTYPE m_port_info;

    OMX_PARAM_PORTDEFINITIONTYPE m_port_def;
    OMX_U32 m_format_index;
    OMX_U32 m_profile_levels_index;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxVideoPortData
{
    MfxOmxPortData m_port_state_data;
    union // decoders, encoders or vpp specific structures with disrespect to concrete codecs
    {
        struct
        {
            // decoder specific structures
            OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS m_intel_avc_decode_settings;
        };
    };
};

#define MfxOmxPortData_h264vd MfxOmxVideoPortData
#define MfxOmxPortData_h265vd MfxOmxVideoPortData
#define MfxOmxPortData_h264ve MfxOmxVideoPortData
#define MfxOmxPortData_mp2vd  MfxOmxVideoPortData
#define MfxOmxPortData_vc1vd  MfxOmxVideoPortData
#define MfxOmxPortData_vp8vd  MfxOmxVideoPortData
#define MfxOmxPortData_vp9vd  MfxOmxVideoPortData
#define MfxOmxPortData_h265ve  MfxOmxVideoPortData

/*------------------------------------------------------------------------------*/

struct MfxOmxPortRegData;

typedef MfxOmxPortData* MfxOmxPortCreateFunc(MfxOmxPortRegData* reg_data);

/*------------------------------------------------------------------------------*/

struct MfxOmxPortRegData
{
    MfxOmxPortId m_port_id;
    OMX_U32 m_port_index;
    OMX_DIRTYPE m_port_direction;
    MfxOmxPortCreateFunc* m_create_func;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxVideoPortRegData
{
    MfxOmxPortRegData m_port_reg_data;

    OMX_U32 m_formats_num;
    OMX_VIDEO_PARAM_PORTFORMATTYPE* m_formats;

    OMX_U32 m_profile_levels_num;
    MfxOmxProfileLevelTable* m_profile_levels;
};

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern MfxOmxPortData* mfx_omx_create_port(MfxOmxPortRegData* reg_data);

/*------------------------------------------------------------------------------*/

extern bool mfx_omx_is_index_valid(OMX_INDEXTYPE index, MfxOmxPortId port_id);

extern void mfx_omx_adjust_port_definition(OMX_PARAM_PORTDEFINITIONTYPE* pPortDef,
                                           mfxFrameInfo* pFrameInfo,
                                           bool isMetadataMode,
                                           bool isANWBuffer);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_PORTS_H__

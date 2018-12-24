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

#include "mfx_omx_structures.h"
#include "mfx_omx_utils.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_structures"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_UNKNOWN 0

/*------------------------------------------------------------------------------*/

struct MfxOmxIntTable
{
    OMX_U32 omx_value;
    mfxU16 mfx_value;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxCodingMimeTable
{
    OMX_VIDEO_CODINGTYPE m_coding_type;
    OMX_STRING m_mime_type;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxColorMimeTable
{
    OMX_COLOR_FORMATTYPE m_color_type;
    OMX_STRING m_mime_type;
};

/*------------------------------------------------------------------------------*/

const mfxU16 TEMPORAL_LAYERS_SCALE_RATIO = 2;

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

const MfxOmxCodingMimeTable g_CodingMimeTable[] =
{
    { OMX_VIDEO_CodingAVC, (OMX_STRING)"video/h264" },
    { OMX_VIDEO_CodingMPEG2, (OMX_STRING)"video/mpeg2" },
    { OMX_VIDEO_CodingWMV, (OMX_STRING)"video/vc1" },
    { OMX_VIDEO_CodingVP8, (OMX_STRING)"video/x-vnd.on2.vp8" },
    { OMX_VIDEO_CodingVP9, (OMX_STRING)"video/x-vnd.on2.vp9" },
    { OMX_VIDEO_CodingVP9, (OMX_STRING)"video/x-vnd.on2.vp9" },
    { static_cast<OMX_VIDEO_CODINGTYPE>(MFX_OMX_VIDEO_CodingHEVC), (OMX_STRING)"video/h265" }
};

/*------------------------------------------------------------------------------*/

const MfxOmxColorMimeTable g_ColorMimeTable[] =
{
    { OMX_COLOR_FormatYUV420Planar, (OMX_STRING)"video/raw" },
    { OMX_COLOR_FormatYUV420SemiPlanar, (OMX_STRING)"video/raw" },
    { (OMX_COLOR_FORMATTYPE)OMX_INTEL_COLOR_Format_NV12, (OMX_STRING)"video/raw" },
    { (OMX_COLOR_FORMATTYPE)OMX_INTEL_COLOR_Format_P10, (OMX_STRING)"video/raw" }
};

/*------------------------------------------------------------------------------*/

const MfxOmxIntTable g_h264_profiles[] =
{
    { OMX_VIDEO_AVCProfileBaseline, MFX_PROFILE_AVC_CONSTRAINED_BASELINE },
    { OMX_VIDEO_AVCProfileMain, MFX_PROFILE_AVC_MAIN },
    { OMX_VIDEO_AVCProfileExtended, MFX_PROFILE_AVC_EXTENDED },
    { OMX_VIDEO_AVCProfileHigh, MFX_PROFILE_AVC_HIGH }
};

const MfxOmxIntTable g_h264_levels[] =
{
    { OMX_VIDEO_AVCLevel1,  MFX_LEVEL_AVC_1 },
    { OMX_VIDEO_AVCLevel1b, MFX_LEVEL_AVC_1b },
    { OMX_VIDEO_AVCLevel11, MFX_LEVEL_AVC_11 },
    { OMX_VIDEO_AVCLevel12, MFX_LEVEL_AVC_12 },
    { OMX_VIDEO_AVCLevel13, MFX_LEVEL_AVC_13 },
    { OMX_VIDEO_AVCLevel2,  MFX_LEVEL_AVC_2 },
    { OMX_VIDEO_AVCLevel21, MFX_LEVEL_AVC_21 },
    { OMX_VIDEO_AVCLevel22, MFX_LEVEL_AVC_22 },
    { OMX_VIDEO_AVCLevel3,  MFX_LEVEL_AVC_3 },
    { OMX_VIDEO_AVCLevel31, MFX_LEVEL_AVC_31 },
    { OMX_VIDEO_AVCLevel31, MFX_LEVEL_AVC_32 },
    { OMX_VIDEO_AVCLevel4,  MFX_LEVEL_AVC_4 },
    { OMX_VIDEO_AVCLevel41, MFX_LEVEL_AVC_41 },
    { OMX_VIDEO_AVCLevel42, MFX_LEVEL_AVC_42 },
    { OMX_VIDEO_AVCLevel5,  MFX_LEVEL_AVC_5 },
    { OMX_VIDEO_AVCLevel51, MFX_LEVEL_AVC_51 }
};

/*------------------------------------------------------------------------------*/

const MfxOmxIntTable g_h265_profiles[] =
{
    { OMX_VIDEO_HEVCProfileMain, MFX_PROFILE_HEVC_MAIN },
    { OMX_VIDEO_HEVCProfileMain10, MFX_PROFILE_HEVC_MAIN10 }
#ifdef HEVC10HDR_SUPPORT
    , { OMX_VIDEO_HEVCProfileMain10HDR10, MFX_PROFILE_HEVC_MAIN10 }
#endif
};

const MfxOmxIntTable g_h265_levels[] =
{
    { OMX_VIDEO_HEVCMainTierLevel1,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_1 },
    { OMX_VIDEO_HEVCHighTierLevel1,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_1 },
    { OMX_VIDEO_HEVCMainTierLevel2,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_2  },
    { OMX_VIDEO_HEVCHighTierLevel2,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_2 },
    { OMX_VIDEO_HEVCMainTierLevel21, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_21  },
    { OMX_VIDEO_HEVCHighTierLevel21, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_21 },
    { OMX_VIDEO_HEVCMainTierLevel3,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_3  },
    { OMX_VIDEO_HEVCHighTierLevel3,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_3 },
    { OMX_VIDEO_HEVCMainTierLevel31, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_31 },
    { OMX_VIDEO_HEVCHighTierLevel31, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_31 },
    { OMX_VIDEO_HEVCMainTierLevel4,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_4 },
    { OMX_VIDEO_HEVCHighTierLevel4,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_4 },
    { OMX_VIDEO_HEVCMainTierLevel41, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_41 },
    { OMX_VIDEO_HEVCHighTierLevel41, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_41 },
    { OMX_VIDEO_HEVCMainTierLevel5,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_5 },
    { OMX_VIDEO_HEVCHighTierLevel5,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_5 },
    { OMX_VIDEO_HEVCMainTierLevel51, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_51 },
    { OMX_VIDEO_HEVCHighTierLevel51, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_51 },
    { OMX_VIDEO_HEVCMainTierLevel52, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_52 },
    { OMX_VIDEO_HEVCHighTierLevel52, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_52 },
    { OMX_VIDEO_HEVCMainTierLevel6,  MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_6 },
    { OMX_VIDEO_HEVCHighTierLevel6,  MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_6 },
    { OMX_VIDEO_HEVCMainTierLevel61, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_61 },
    { OMX_VIDEO_HEVCHighTierLevel61, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_61 },
    { OMX_VIDEO_HEVCMainTierLevel62, MFX_TIER_HEVC_MAIN + MFX_LEVEL_HEVC_62 },
    { OMX_VIDEO_HEVCHighTierLevel62, MFX_TIER_HEVC_HIGH + MFX_LEVEL_HEVC_62 },
};

/*------------------------------------------------------------------------------*/

const MfxOmxIntTable g_brc_method[] =
{
    { OMX_Video_ControlRateDisable,                     MFX_RATECONTROL_CQP },
    { OMX_Video_ControlRateVariable,                    MFX_RATECONTROL_VBR },
    { OMX_Video_ControlRateConstant,                    MFX_RATECONTROL_CBR },
    { OMX_Video_ControlRateVariableSkipFrames,          MFX_RATECONTROL_VBR },
    { OMX_Video_ControlRateConstantSkipFrames,          MFX_RATECONTROL_CBR },
    { OMX_Video_ControlRateMax,                         MFX_RATECONTROL_CQP },
    { OMX_Video_Intel_ControlRateVideoConferencingMode, MFX_RATECONTROL_VBR },
};

/*------------------------------------------------------------------------------*/

OMX_STRING mfx_omx_coding2mime_type(OMX_VIDEO_CODINGTYPE type)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_U32 i = 0;

    if (OMX_VIDEO_CodingUnused == type) return NULL;
    for (i = 0; i < MFX_OMX_GET_ARRAY_SIZE(g_CodingMimeTable); ++i)
    {
        if (g_CodingMimeTable[i].m_coding_type == type) break;
    }
    if (i >= MFX_OMX_GET_ARRAY_SIZE(g_CodingMimeTable)) return NULL;
    return g_CodingMimeTable[i].m_mime_type;
}

/*------------------------------------------------------------------------------*/

OMX_STRING mfx_omx_color2mime_type(OMX_COLOR_FORMATTYPE type)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_U32 i = 0;

    if (OMX_COLOR_FormatUnused == type) return NULL;
    for (i = 0; i < MFX_OMX_GET_ARRAY_SIZE(g_ColorMimeTable); ++i)
    {
        if (g_ColorMimeTable[i].m_color_type == type) break;
    }
    if (i >= MFX_OMX_GET_ARRAY_SIZE(g_ColorMimeTable)) return NULL;
    return g_ColorMimeTable[i].m_mime_type;
}

/*------------------------------------------------------------------------------*/

inline OMX_U32 mfx2omx_int_table_value(const MfxOmxIntTable* table, mfxU32 table_size, mfxU16 mfx_value)
{
    mfxU32 i = 0;

    for (i = 0; i < table_size; ++i)
    {
        if (table[i].mfx_value == mfx_value) return table[i].omx_value;
    }
    return MFX_OMX_UNKNOWN;
}

/*------------------------------------------------------------------------------*/

inline mfxU16 omx2mfx_int_table_value(const MfxOmxIntTable* table, mfxU32 table_size, OMX_U32 omx_value)
{
    mfxU32 i = 0;

    for (i = 0; i < table_size; ++i)
    {
        if (table[i].omx_value == omx_value) return table[i].mfx_value;
    }
    return MFX_OMX_UNKNOWN;
}

/*------------------------------------------------------------------------------*/

OMX_VIDEO_AVCPROFILETYPE mfx2omx_avc_profile(mfxU16 mfx_profile)
{
    return (OMX_VIDEO_AVCPROFILETYPE)mfx2omx_int_table_value(g_h264_profiles,
                                                             MFX_OMX_GET_ARRAY_SIZE(g_h264_profiles),
                                                             mfx_profile);
}

/*------------------------------------------------------------------------------*/

mfxU16 omx2mfx_avc_profile(OMX_U32 omx_profile)
{
    return omx2mfx_int_table_value(g_h264_profiles,
                                   MFX_OMX_GET_ARRAY_SIZE(g_h264_profiles),
                                   omx_profile);
}

/*------------------------------------------------------------------------------*/

OMX_VIDEO_AVCLEVELTYPE mfx2omx_avc_level(mfxU16 mfx_level)
{
    return (OMX_VIDEO_AVCLEVELTYPE)mfx2omx_int_table_value(g_h264_levels,
                                                           MFX_OMX_GET_ARRAY_SIZE(g_h264_levels),
                                                           mfx_level);
}

/*------------------------------------------------------------------------------*/

mfxU16 omx2mfx_avc_level(OMX_U32 omx_level)
{
    return omx2mfx_int_table_value(g_h264_levels,
                                   MFX_OMX_GET_ARRAY_SIZE(g_h264_levels),
                                   omx_level);
}

/*------------------------------------------------------------------------------*/

OMX_VIDEO_HEVCPROFILETYPE mfx2omx_hevc_profile(mfxU16 mfx_profile)
{
    return (OMX_VIDEO_HEVCPROFILETYPE)mfx2omx_int_table_value(g_h265_profiles,
                                                             MFX_OMX_GET_ARRAY_SIZE(g_h265_profiles),
                                                             mfx_profile);
}

/*------------------------------------------------------------------------------*/

mfxU16 omx2mfx_hevc_profile(OMX_U32 omx_profile)
{
    return omx2mfx_int_table_value(g_h265_profiles,
                                   MFX_OMX_GET_ARRAY_SIZE(g_h265_profiles),
                                   omx_profile);
}

/*------------------------------------------------------------------------------*/

OMX_VIDEO_HEVCLEVELTYPE mfx2omx_hevc_level(mfxU16 mfx_level)
{
    return (OMX_VIDEO_HEVCLEVELTYPE)mfx2omx_int_table_value(g_h265_levels,
                                                           MFX_OMX_GET_ARRAY_SIZE(g_h265_levels),
                                                           mfx_level);
}

/*------------------------------------------------------------------------------*/

mfxU16 omx2mfx_hevc_level(OMX_U32 omx_level)
{
    return omx2mfx_int_table_value(g_h265_levels,
                                   MFX_OMX_GET_ARRAY_SIZE(g_h265_levels),
                                   omx_level);
}
/*------------------------------------------------------------------------------*/

OMX_VIDEO_CONTROLRATETYPE mfx2omx_brc(mfxU16 mfx_brc)
{
    return (OMX_VIDEO_CONTROLRATETYPE)mfx2omx_int_table_value(g_brc_method,
                                                              MFX_OMX_GET_ARRAY_SIZE(g_brc_method),
                                                              mfx_brc);
}

/*------------------------------------------------------------------------------*/

mfxU16 omx2mfx_brc(OMX_VIDEO_CONTROLRATETYPE omx_brc)
{
    return omx2mfx_int_table_value(g_brc_method,
                                   MFX_OMX_GET_ARRAY_SIZE(g_brc_method),
                                   omx_brc);
}

/*------------------------------------------------------------------------------*/

MfxOmxEncodeCtrlWrapper* CreateEncodeCtrlWrapper()
{
    MfxOmxEncodeCtrlWrapper* wrap = NULL;

    MFX_OMX_NEW(wrap,
                MfxOmxEncodeCtrlWrapper);
    return wrap;
}

/*------------------------------------------------------------------------------*/

MfxOmxVideoParamsWrapper* CreateVideoParamsWrapper(
    const MfxOmxVideoParamsWrapper& ref,
    bool bNeedResetExtBuffer
)
{
    MfxOmxVideoParamsWrapper* wrap = NULL;

    MFX_OMX_NEW(wrap,
                MfxOmxVideoParamsWrapper(ref));
    if (wrap && bNeedResetExtBuffer && (MFX_CODEC_AVC == ref.mfx.CodecId))
    {
        int idx = wrap->enableExtParam(MFX_EXTBUFF_ENCODER_RESET_OPTION);
        if (idx < 0 || idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
             MFX_OMX_DELETE(wrap);
        }
        else
        {
            wrap->ext_buf[idx].reset.StartNewSequence = MFX_CODINGOPTION_UNKNOWN;
        }
    }
    return wrap;
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

template<typename T>
inline void mfx_omx_reset_omx_structure(T& omxparams)
{
    OMX_U32 nSize = omxparams.nSize;
    OMX_VERSIONTYPE nVersion = omxparams.nVersion;
    OMX_U32 nPortIndex = omxparams.nPortIndex;

    MFX_OMX_ZERO_MEMORY(omxparams);

    omxparams.nSize = nSize;
    omxparams.nVersion = nVersion;
    omxparams.nPortIndex = nPortIndex;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxEncodeCtrlWrapper& mfxctrl,
    const OMX_VIDEO_CONFIG_USERDATA& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AT__OMX_VIDEO_ENCODER_USER_DATA(omxparams);

    if (omxparams.nUserDataSize <= 0 || omxparams.pUserData == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }

    mfxctrl.payload = (mfxPayload*)calloc(1, sizeof(mfxPayload));
    if(!mfxctrl.payload)
    {
        return OMX_ErrorInsufficientResources;
    }
    mfxI32 numFbytes = (16 + omxparams.nUserDataSize) / 256; // calculate number of F bytes in payload size
    mfxctrl.payload->BufSize =
        1 + // 1 byte PayloadType for User Data Unregistered
        numFbytes +
        1 + //1 byte PayloadSize w/o PayloadType and PayloadSize bytes
        16 + // UUID
        omxparams.nUserDataSize; // user data size

    MFX_OMX_AUTO_TRACE_I32(mfxctrl.payload->BufSize);
    mfxctrl.payload->Data = (mfxU8*)calloc(1, mfxctrl.payload->BufSize);
    if(!mfxctrl.payload->Data)
    {
        MFX_OMX_FREE(mfxctrl.payload);
        return OMX_ErrorInsufficientResources;
    }
    //pack userdata to the payload.
    mfxU8 SEI_UUID[16] = {0xBC, 0x64, 0x79, 0x30, 0xBF, 0xF9, 0x11, 0xE3, 0x8A, 0x33, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66};
    int index = 0;
    mfxctrl.payload->Data[index++] = MFX_OMX_SEI_USER_DATA_UNREGISTERED_TYPE; //PayloadType = 5 for User Data Unregistered
    for(int i = 0; i < numFbytes; i++) // Add numFbytes number of 0xF bytes
    {
        mfxctrl.payload->Data[index++] = 0xFF;
    }
    mfxctrl.payload->Data[index++] = (16 + omxparams.nUserDataSize) % 256; // PayloadSize w/o PayloadType and PayloadSize bytes where is 16 (UUID size)
    std::copy(std::begin(SEI_UUID), std::end(SEI_UUID), &(mfxctrl.payload->Data[index])); // copy UUID
    index += 16;
    std::copy(omxparams.pUserData, omxparams.pUserData + omxparams.nUserDataSize, &(mfxctrl.payload->Data[index])); // copy userdata

    MFX_OMX_AUTO_TRACE_I32(index);

    mfxctrl.payload->Type = MFX_OMX_SEI_USER_DATA_UNREGISTERED_TYPE;
    mfxctrl.payload->NumBit = 8 * mfxctrl.payload->BufSize;
    mfxctrl.Payload = &mfxctrl.payload;
    mfxctrl.NumPayload = 1;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
        MfxOmxInputConfig& config,
        const OMX_VIDEO_CONFIG_OPERATION_RATE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(config);
    MFX_OMX_UNUSED(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
        MfxOmxInputConfig& config,
        const OMX_VIDEO_CONFIG_PRIORITY& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(config);
    MFX_OMX_UNUSED(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxEncodeCtrlWrapper& mfxctrl,
    const OMX_VIDEO_CONFIG_DIRTY_RECT& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (omxparams.nNumRectangles <= 0 || omxparams.pRectangles == NULL)
    {
        return OMX_ErrorInsufficientResources;
    }
    MFX_OMX_AT__OMX_VIDEO_ENCODER_DIRTY_RECT(omxparams);

    int idx = mfxctrl.enableExtParam(MFX_EXTBUFF_ENCODER_ROI);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    mfxctrl.ext_buf[idx].roi.NumROI = omxparams.nNumRectangles;

    for(OMX_U32 i = 0; i < omxparams.nNumRectangles; i++)
    {
        mfxctrl.ext_buf[idx].roi.ROI[i].Left = omxparams.pRectangles[i].nLeft;
        mfxctrl.ext_buf[idx].roi.ROI[i].Top = omxparams.pRectangles[i].nTop;
        mfxctrl.ext_buf[idx].roi.ROI[i].Right = omxparams.pRectangles[i].nLeft + omxparams.pRectangles[i].nWidth;
        mfxctrl.ext_buf[idx].roi.ROI[i].Bottom = omxparams.pRectangles[i].nTop + omxparams.pRectangles[i].nHeight;
    }

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxEncodeCtrlWrapper& mfxctrl,
    const OMX_CONFIG_INTRAREFRESHVOPTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(INTRAREFRESHVOPTYPE): IntraRefreshVOP %d", omxparams.IntraRefreshVOP);
    MFX_OMX_AT__OMX_CONFIG_INTRAREFRESHVOPTYPE(omxparams);

    if (omxparams.IntraRefreshVOP == OMX_TRUE)
    {
        mfxctrl.FrameType =
            MFX_FRAMETYPE_I |
            MFX_FRAMETYPE_IDR |
            MFX_FRAMETYPE_REF;
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/


template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxEncodeCtrlWrapper& mfxctrl,
    const OMX_CONFIG_DUMMYFRAME& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AT__OMX_CONFIG_DUMMYFRAME(omxparams);
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(DUMMYFRAME): bInsertDummyFrame %d", omxparams.bInsertDummyFrame);

    if (omxparams.bInsertDummyFrame == OMX_TRUE)
    {
        mfxI32 idx = mfxctrl.enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
        if (idx >= 0 && idx < MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            mfxctrl.ext_buf[idx].opt2.SkipFrame = MFX_SKIPFRAME_INSERT_DUMMY;
            mfxctrl.SkipFrame = true;
        }
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_NUMSLICE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(NUMSLICE): nSliceNumber %d\n", omxparams.nSliceNumber);

    MFX_OMX_AT__OMX_CONFIG_NUMSLICE(omxparams);
    config.mfxparams->mfx.NumSlice = omxparams.nSliceNumber;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_BITRATELIMITOFF& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(BITRATELIMITOFF): bBitrateLimitOff %d\n", omxparams.bBitrateLimitOff);

    MFX_OMX_AT__OMX_CONFIG_BITRATELIMITOFF(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    config.mfxparams->ext_buf[idx].opt2.BitrateLimit = MFX_CODINGOPTION_OFF;
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_NUMREFFRAME& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(NUMREFFRAME): nRefFrame %d\n", omxparams.nRefFrame);

    MFX_OMX_AT__OMX_CONFIG_NUMREFFRAME(omxparams);
    config.mfxparams->mfx.NumRefFrame = omxparams.nRefFrame;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_GOPPICSIZE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(OMX_CONFIG_GOPPICSIZE): nGopPicSize %d\n", omxparams.nGopPicSize);

    MFX_OMX_AT__OMX_CONFIG_GOPPICSIZE(omxparams);
    config.mfxparams->mfx.GopPicSize = omxparams.nGopPicSize;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_LOWPOWER& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(OMX_CONFIG_LOWPOWER): nLowPower %d\n", omxparams.bLowPower);
    MFX_OMX_AT__OMX_CONFIG_LOWPOWER(omxparams);

    if (OMX_TRUE == omxparams.bLowPower)
        config.mfxparams->mfx.LowPower = MFX_CODINGOPTION_ON;
    else
        config.mfxparams->mfx.LowPower = MFX_CODINGOPTION_OFF;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_IDRINTERVAL& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(OMX_CONFIG_IDRINTERVAL): nIdrInterval %d\n", omxparams.nIdrInterval);

    MFX_OMX_AT__OMX_CONFIG_IDRINTERVAL(omxparams);
    config.mfxparams->mfx.IdrInterval  = omxparams.nIdrInterval;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_GOPREFDIST& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(OMX_CONFIG_GOPREFDIST&): nGopRefDist %d\n", omxparams.nGopRefDist);

    MFX_OMX_AT__OMX_CONFIG_GOPREFDIST(omxparams);
    config.mfxparams->mfx.GopRefDist = omxparams.nGopRefDist;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_DISABLEDEBLOCKINGIDC& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(DISABLEDEBLOCKINGIDC): bDisableDeblockingIdc %d\n", omxparams.bDisableDeblockingIdc);

    MFX_OMX_AT__OMX_CONFIG_DISABLEDEBLOCKINGIDC(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    config.mfxparams->ext_buf[idx].opt2.DisableDeblockingIdc = omxparams.bDisableDeblockingIdc;
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_INTEL_BITRATETYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(INTEL_BITRATETYPE): nInitialQP %d, nMaxEncodeBitrate %d, nTargetPercentage %d, nMinQP %d, nMaxQP %d",
                        omxparams.nInitialQP, omxparams.nMaxEncodeBitrate, omxparams.nTargetPercentage, omxparams.nMinQP, omxparams.nMaxQP);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_BITRATETYPE(omxparams);

    if (MFX_RATECONTROL_CQP == config.mfxparams->mfx.RateControlMethod)
    {
        if (!config.control) config.control = CreateEncodeCtrlWrapper();
        if (!config.control) return OMX_ErrorInsufficientResources;

        config.control->QP = (mfxU8)(omxparams.nInitialQP);
    }
    else
    {
        config.mfxparams->mfx.InitialDelayInKB = 0;
        config.mfxparams->mfx.BufferSizeInKB = 0;

        config.mfxparams->mfx.MaxKbps = (mfxU16)(omxparams.nMaxEncodeBitrate * 0.001);
        config.mfxparams->mfx.TargetKbps = (mfxU16)((config.mfxparams->mfx.MaxKbps * omxparams.nTargetPercentage) / 100.0);

        // NOTE nFrameRate comes as 0 -- ignoring this
        //config.mfxparams->mfx.FrameInfo.FrameRateExtN = (mfxU32)omxparams.nFrameRate;
        //config.mfxparams->mfx.FrameInfo.FrameRateExtD = 1;

        // NOTE mfx.Convergence vs. mfx.MaxKbps -- conflict in mfx API
        //OMX_U32 convergenceFrames = (config.mfxparams->mfx.FrameInfo.FrameRateExtN * omxparams.nWindowSize + 999) / 1000;
        //config.mfxparams->mfx.Convergence = (convergenceFrames + 99.0) / 100;

        // NOTE nTemporalID comes as incorrect value

        if (MFX_CODEC_AVC == config.mfxparams->mfx.CodecId)
        {
            int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
            if (idx < 0)
            {
                return OMX_ErrorInsufficientResources;
            }
            else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
            {
                return OMX_ErrorNoMore;
            }

            if (omxparams.nMaxQP > omxparams.nMinQP)
            {
                config.mfxparams->ext_buf[idx].opt2.MinQPI = (mfxU8)(omxparams.nMinQP);
                config.mfxparams->ext_buf[idx].opt2.MaxQPI = (mfxU8)(omxparams.nMaxQP);

                config.mfxparams->ext_buf[idx].opt2.MinQPP = (mfxU8)(omxparams.nMinQP);
                config.mfxparams->ext_buf[idx].opt2.MaxQPP = (mfxU8)(omxparams.nMaxQP);

                config.mfxparams->ext_buf[idx].opt2.MinQPB = (mfxU8)(omxparams.nMinQP);
                config.mfxparams->ext_buf[idx].opt2.MaxQPB = (mfxU8)(omxparams.nMaxQP);
            }
        }
    }


    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_CONFIG_INTEL_BITRATETYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nMaxEncodeBitrate = mfxparams.mfx.MaxKbps * 1000;
    omxparams.nTargetPercentage = mfxparams.mfx.TargetKbps * 100.0 / mfxparams.mfx.MaxKbps + 0.5;
    if (omxparams.nTargetPercentage > 100) omxparams.nTargetPercentage = 100;

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx >= 0)
    {
        omxparams.nMinQP = mfxparams.ext_buf[idx].opt2.MinQPI;
    }

    MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_BITRATETYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(INTEL_SLICE_NUMBERS): nISliceNumber %d, nPSliceNumber %d",
                        omxparams.nISliceNumber, omxparams.nPSliceNumber);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION3);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    config.mfxparams->ext_buf[idx].opt3.NumSliceI = (mfxU16)(omxparams.nISliceNumber);
    config.mfxparams->ext_buf[idx].opt3.NumSliceP = (mfxU16)(omxparams.nPSliceNumber);
    config.mfxparams->ext_buf[idx].opt3.NumSliceB = 1;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    // default values
    omxparams.nISliceNumber = mfxparams.mfx.NumSlice;
    omxparams.nPSliceNumber = mfxparams.mfx.NumSlice;

    // corrected values by ext buffer
    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION3);
    if (idx >= 0)
    {
        omxparams.nISliceNumber = mfxparams.ext_buf[idx].opt3.NumSliceI;
        omxparams.nPSliceNumber = mfxparams.ext_buf[idx].opt3.NumSliceP;
    }

    MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_AVCINTRAPERIOD& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(AVCINTRAPERIOD): nPFrames %d, nIDRPeriod %d",
                        omxparams.nPFrames, omxparams.nIDRPeriod);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_AVCINTRAPERIOD(omxparams);

    config.mfxparams->mfx.GopPicSize = omxparams.nPFrames + 1;
    config.mfxparams->mfx.IdrInterval = (omxparams.nIDRPeriod)? omxparams.nIDRPeriod - 1: 0xFFFF;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_CONFIG_AVCINTRAPERIOD& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nPFrames = (mfxparams.mfx.GopPicSize)? mfxparams.mfx.GopPicSize - 1: 0;
    omxparams.nIDRPeriod = mfxparams.mfx.IdrInterval + 1;

    MFX_OMX_AT__OMX_VIDEO_CONFIG_AVCINTRAPERIOD(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_AVCTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(AVCTYPE): eProfile %d, eLevel %d, nPFrames %d, nBFrames %d, nRefFrames %d, bEntropyCodingCABAC %d",
                        omxparams.eProfile, omxparams.eLevel, omxparams.nPFrames, omxparams.nBFrames, omxparams.nRefFrames, omxparams.bEntropyCodingCABAC);
    MFX_OMX_AT__OMX_VIDEO_PARAM_AVCTYPE(omxparams);

    config.mfxparams->mfx.CodecProfile = omx2mfx_avc_profile(omxparams.eProfile);
    config.mfxparams->mfx.CodecLevel = omx2mfx_avc_level(omxparams.eLevel);
    config.mfxparams->mfx.GopPicSize = omxparams.nPFrames + omxparams.nBFrames + 1;
    config.mfxparams->mfx.GopRefDist = omxparams.nBFrames + 1;
    config.mfxparams->mfx.NumRefFrame = omxparams.nRefFrames;

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    if (OMX_TRUE == omxparams.bEntropyCodingCABAC)
        config.mfxparams->ext_buf[idx].opt.CAVLC = MFX_CODINGOPTION_OFF;
    else
        config.mfxparams->ext_buf[idx].opt.CAVLC = MFX_CODINGOPTION_ON;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_AVCTYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.eProfile = mfx2omx_avc_profile(mfxparams.mfx.CodecProfile);
    omxparams.eLevel = mfx2omx_avc_level(mfxparams.mfx.CodecLevel);
    omxparams.nBFrames = mfxparams.mfx.GopRefDist - 1;
    omxparams.nPFrames = mfxparams.mfx.GopPicSize - mfxparams.mfx.GopRefDist;
    omxparams.nRefFrames = mfxparams.mfx.NumRefFrame;

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    if (MFX_CODINGOPTION_OFF == mfxparams.ext_buf[idx].opt.CAVLC)
        omxparams.bEntropyCodingCABAC = OMX_TRUE;

    MFX_OMX_AT__OMX_VIDEO_PARAM_AVCTYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_HEVCTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_AT__OMX_VIDEO_PARAM_HEVCTYPE(omxparams);

    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(HEVCTYPE): eProfile %X, eLevel %X nKeyFrameInterval %d", omxparams.eProfile, omxparams.eLevel, omxparams.nKeyFrameInterval);
    config.mfxparams->mfx.GopPicSize = omxparams.nKeyFrameInterval;
    config.mfxparams->mfx.CodecProfile = omx2mfx_hevc_profile(omxparams.eProfile);
    config.mfxparams->mfx.CodecLevel = omx2mfx_hevc_level(omxparams.eLevel);

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_HEVCTYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.eProfile = mfx2omx_hevc_profile(mfxparams.mfx.CodecProfile);
    omxparams.eLevel = mfx2omx_hevc_level(mfxparams.mfx.CodecLevel);
    omxparams.nKeyFrameInterval = mfxparams.mfx.GopPicSize;

    MFX_OMX_AT__OMX_VIDEO_PARAM_HEVCTYPE(omxparams);
    return OMX_ErrorNone;
}


/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_PROFILELEVELTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(PROFILELEVELTYPE): eProfile %d, eLevel %d", omxparams.eProfile, omxparams.eLevel);
    MFX_OMX_AT__OMX_VIDEO_PARAM_PROFILELEVELTYPE(omxparams);

    config.mfxparams->mfx.CodecProfile = omx2mfx_avc_profile(omxparams.eProfile);
    config.mfxparams->mfx.CodecLevel = omx2mfx_avc_level(omxparams.eLevel);

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_PROFILELEVELTYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.eProfile = mfx2omx_avc_profile(mfxparams.mfx.CodecProfile);
    omxparams.eLevel = mfx2omx_avc_level(mfxparams.mfx.CodecLevel);

    MFX_OMX_AT__OMX_VIDEO_PARAM_PROFILELEVELTYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_BITRATETYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(BITRATETYPE): nTargetBitrate %d, eControlRate %d", omxparams.nTargetBitrate, omxparams.eControlRate);
    MFX_OMX_AT__OMX_VIDEO_PARAM_BITRATETYPE(omxparams);

    // OMX standart - bitrate in bps, MFX - in kbps
    config.mfxparams->mfx.TargetKbps = (mfxU16)(omxparams.nTargetBitrate * 0.001);
    config.mfxparams->mfx.MaxKbps = 0;
    config.mfxparams->mfx.RateControlMethod = omx2mfx_brc(omxparams.eControlRate);

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_BITRATETYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nTargetBitrate = mfxparams.mfx.TargetKbps * 1000;
    omxparams.eControlRate = mfx2omx_brc(mfxparams.mfx.RateControlMethod);

    MFX_OMX_AT__OMX_VIDEO_PARAM_BITRATETYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_BITRATETYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(BITRATETYPE): nEncodeBitrate %d", omxparams.nEncodeBitrate);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_BITRATETYPE(omxparams);

    // OMX standart - bitrate in bps, MFX - in kbps
    config.mfxparams->mfx.TargetKbps = (mfxU16)(omxparams.nEncodeBitrate * 0.001);
    config.mfxparams->mfx.MaxKbps = 0;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_CONFIG_BITRATETYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nEncodeBitrate = mfxparams.mfx.TargetKbps * 1000;

    MFX_OMX_AT__OMX_VIDEO_CONFIG_BITRATETYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_INTRAREFRESHTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(INTRAREFRESHTYPE): nCirMBs %d", omxparams.nCirMBs);
    MFX_OMX_AT__OMX_VIDEO_PARAM_INTRAREFRESHTYPE(omxparams);

    if (omxparams.nCirMBs)
    {
        OMX_U32 wMBs = config.mfxparams->mfx.FrameInfo.Width >> 4;
        OMX_U32 hMBs = config.mfxparams->mfx.FrameInfo.Height >> 4;
        OMX_U32 nCirMBVert =  omxparams.nCirMBs < wMBs * hMBs ? (omxparams.nCirMBs >= hMBs ? omxparams.nCirMBs / hMBs : 1) : hMBs - 1;

        int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
        if (idx < 0)
        {
            return OMX_ErrorInsufficientResources;
        }
        else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            return OMX_ErrorNoMore;
        }

        config.mfxparams->ext_buf[idx].opt2.IntRefType = 1; // VERT_REFRESH
        config.mfxparams->ext_buf[idx].opt2.IntRefCycleSize = (wMBs + nCirMBVert - 1) / nCirMBVert;
        config.mfxparams->ext_buf[idx].opt2.IntRefQPDelta = 0;
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "omx2mfx: IntRefCycleSize= %d", config.mfxparams->ext_buf[idx].opt2.IntRefCycleSize);
    }

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_INTRAREFRESHTYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx >= 0)
    {
        OMX_U32 wMBs = mfxparams.mfx.FrameInfo.Width >> 4;
        OMX_U32 hMBs = mfxparams.mfx.FrameInfo.Height >> 4;

        if (mfxparams.ext_buf[idx].opt2.IntRefCycleSize)
            omxparams.nCirMBs = wMBs * (1 + (hMBs-1)/mfxparams.ext_buf[idx].opt2.IntRefCycleSize);
        else
            omxparams.nCirMBs = 0;
    }

    MFX_OMX_AT__OMX_VIDEO_PARAM_INTRAREFRESHTYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE): nRefreshPeriod %d", omxparams.nRefreshPeriod);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    config.mfxparams->ext_buf[idx].opt2.IntRefType = (omxparams.nRefreshPeriod != 0) ? MFX_REFRESH_HORIZONTAL : MFX_REFRESH_NO;
    config.mfxparams->ext_buf[idx].opt2.IntRefCycleSize = omxparams.nRefreshPeriod;
    config.mfxparams->ext_buf[idx].opt2.IntRefQPDelta = 0;

    // GopPicSize must be greater than Intra Refresh Cycle Size. By default GopPicSize == 15 or 16
    // In first call Query if IntRefCycleSize >= GopPicSize then we reset IntRefCycleSize and IntRefType to 0, that means disabling intra refresh
    if (config.mfxparams->mfx.GopPicSize <= config.mfxparams->ext_buf[idx].opt2.IntRefCycleSize)
    {
        config.mfxparams->mfx.GopPicSize = config.mfxparams->ext_buf[idx].opt2.IntRefCycleSize + 1;
    }

    return OMX_ErrorNone;
}

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfx_omx_reset_omx_structure(omxparams);

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx >= 0)
    {
        omxparams.nRefreshPeriod = mfxparams.ext_buf[idx].opt2.IntRefCycleSize;
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_CONFIG_FRAMERATETYPE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetConfig(FRAMERATETYPE): xEncodeFramerate %x", omxparams.xEncodeFramerate);
    MFX_OMX_AT__OMX_CONFIG_FRAMERATETYPE(omxparams);

    config.mfxparams->mfx.FrameInfo.FrameRateExtN = (mfxU32)(omxparams.xEncodeFramerate >> 16);
    config.mfxparams->mfx.FrameInfo.FrameRateExtD = 1;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_CONFIG_FRAMERATETYPE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.xEncodeFramerate = mfxparams.mfx.FrameInfo.FrameRateExtN << 16;

    MFX_OMX_AT__OMX_CONFIG_FRAMERATETYPE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_INTEL_AVCVUI& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(OMX_VIDEO_PARAM_INTEL_AVCVUI): bVuiGeneration %d", omxparams.bVuiGeneration);
    MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVCVUI(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    if (omxparams.bVuiGeneration)
        config.mfxparams->ext_buf[idx].opt2.DisableVUI = MFX_CODINGOPTION_OFF;
    else
        config.mfxparams->ext_buf[idx].opt2.DisableVUI = MFX_CODINGOPTION_ON;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_INTEL_AVCVUI& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        // by default Media SDK generates VUI
        omxparams.bVuiGeneration = OMX_TRUE;
    }
    else
    {
        if (mfxparams.ext_buf[idx].opt2.DisableVUI == MFX_CODINGOPTION_OFF)
            omxparams.bVuiGeneration = OMX_TRUE;
        else
            omxparams.bVuiGeneration = OMX_FALSE;
    }

    MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVCVUI(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_HRD_PARAM& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(HRD_PARAM): nHRDBufferSize %d, nHRDInitialFullness %d, bWriteHRDSyntax %d",
                        omxparams.nHRDBufferSize, omxparams.nHRDInitialFullness, omxparams.bWriteHRDSyntax);
    MFX_OMX_AT__OMX_VIDEO_PARAM_HRD_PARAM(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    config.mfxparams->mfx.BufferSizeInKB = omxparams.nHRDBufferSize;
    config.mfxparams->mfx.InitialDelayInKB = omxparams.nHRDInitialFullness;
    config.mfxparams->ext_buf[idx].opt.NalHrdConformance = MFX_CODINGOPTION_ON;
    if (OMX_TRUE == omxparams.bWriteHRDSyntax)
        config.mfxparams->ext_buf[idx].opt.VuiNalHrdParameters = MFX_CODINGOPTION_ON;
    else
        config.mfxparams->ext_buf[idx].opt.VuiNalHrdParameters = MFX_CODINGOPTION_OFF;

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_HRD_PARAM& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    omxparams.nHRDBufferSize = mfxparams.mfx.BufferSizeInKB;
    omxparams.nHRDInitialFullness = mfxparams.mfx.InitialDelayInKB;
    if (MFX_CODINGOPTION_ON == mfxparams.ext_buf[idx].opt.VuiNalHrdParameters)
        omxparams.bWriteHRDSyntax = OMX_TRUE;

    MFX_OMX_AT__OMX_VIDEO_PARAM_HRD_PARAM(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_MAX_PICTURE_SIZE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(MAX_PICTURE_SIZE): nMaxPictureSize %d", omxparams.nMaxPictureSize);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_MAX_PICTURE_SIZE(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    if (omxparams.nMaxPictureSize > 0)
        config.mfxparams->ext_buf[idx].opt2.MaxFrameSize = omxparams.nMaxPictureSize; // in bytes

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_MAX_PICTURE_SIZE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        // by default return 0
        omxparams.nMaxPictureSize = 0;
    }
    else
    {
        omxparams.nMaxPictureSize = mfxparams.ext_buf[idx].opt2.MaxFrameSize;
    }

    MFX_OMX_AT__OMX_VIDEO_CONFIG_MAX_PICTURE_SIZE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_TARGET_USAGE& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(TARGET_USAGE): nTargetUsage %d", omxparams.nTargetUsage);
    MFX_OMX_AT__OMX_VIDEO_CONFIG_TARGET_USAGE(omxparams);

    if (omxparams.nTargetUsage <= 7)
    {
        config.mfxparams->mfx.TargetUsage = omxparams.nTargetUsage;
    }
    else
    {
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_TARGET_USAGE& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nTargetUsage = mfxparams.mfx.TargetUsage;

    MFX_OMX_AT__OMX_VIDEO_CONFIG_TARGET_USAGE(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(TARGET_USAGE): nCropX %d, nCropY %d, nCropW %d, nCropH %d",
                        omxparams.nCropX, omxparams.nCropY, omxparams.nCropW, omxparams.nCropH);
    MFX_OMX_AT__OMX_VIDEO_ENCODER_FRAME_CROPPING(omxparams);

    if (omxparams.nCropW > 0 && omxparams.nCropH > 0)
    {
        config.mfxparams->mfx.FrameInfo.CropX = omxparams.nCropX;
        config.mfxparams->mfx.FrameInfo.CropY = omxparams.nCropY;
        config.mfxparams->mfx.FrameInfo.CropW = omxparams.nCropW;
        config.mfxparams->mfx.FrameInfo.CropH = omxparams.nCropH;
    }
    else
    {
        return OMX_ErrorBadParameter;
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nCropX = mfxparams.mfx.FrameInfo.CropX;
    omxparams.nCropY = mfxparams.mfx.FrameInfo.CropY;
    omxparams.nCropW = mfxparams.mfx.FrameInfo.CropW;
    omxparams.nCropH = mfxparams.mfx.FrameInfo.CropH;

    MFX_OMX_AT__OMX_VIDEO_ENCODER_FRAME_CROPPING(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "SetParam(VUI_CONTROL_PARAM): nAspectRatioW %d, nAspectRatioH %d, nVideoFormat %d, nColourDescriptionPresent %d, nColourPrimaries %d, bVideoFullRange %d",
                        omxparams.nAspectRatioW, omxparams.nAspectRatioH, omxparams.nVideoFormat, omxparams.nColourDescriptionPresent, omxparams.nColourPrimaries, omxparams.bVideoFullRange);
    MFX_OMX_AT__OMX_VIDEO_ENCODER_VUI_CONTROL(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }

    config.mfxparams->mfx.FrameInfo.AspectRatioW = omxparams.nAspectRatioW;
    config.mfxparams->mfx.FrameInfo.AspectRatioH = omxparams.nAspectRatioH;

    config.mfxparams->ext_buf[idx].vsi.VideoFormat = omxparams.nVideoFormat;
    config.mfxparams->ext_buf[idx].vsi.ColourDescriptionPresent = omxparams.nColourDescriptionPresent;
    config.mfxparams->ext_buf[idx].vsi.ColourPrimaries = omxparams.nColourPrimaries;
    config.mfxparams->ext_buf[idx].vsi.VideoFullRange =
        omxparams.bVideoFullRange == 0 ? 0 : 1;

    idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    config.mfxparams->ext_buf[idx].opt2.FixedFrameRate =
        omxparams.nFixedFrameRate == 0 ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON; // NOTE: There is no VA API now the feature

    idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    config.mfxparams->ext_buf[idx].opt.PicTimingSEI = omxparams.nPicStructPresent == 0 ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON;

    idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_CODING_OPTION3);
    if (idx < 0)
    {
        return OMX_ErrorInsufficientResources;
    }
    else if (idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
    {
        return OMX_ErrorNoMore;
    }
    config.mfxparams->ext_buf[idx].opt3.LowDelayHrd = omxparams.nLowDelayHRD == 0 ? MFX_CODINGOPTION_OFF : MFX_CODINGOPTION_ON;

    if (config.bCodecInitialized)
    {
         // request to add IDR frame to write VUI control
        idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_ENCODER_RESET_OPTION);
        if (idx < 0 || idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
        {
            return OMX_ErrorInsufficientResources;
        }
        else
        {
            config.mfxparams->ext_buf[idx].reset.StartNewSequence = MFX_CODINGOPTION_ON;
        }
    }
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE mfx2omx_config(
    OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM& omxparams,
    const MfxOmxVideoParamsWrapper& mfxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfx_omx_reset_omx_structure(omxparams);

    omxparams.nAspectRatioW = mfxparams.mfx.FrameInfo.AspectRatioW;
    omxparams.nAspectRatioH = mfxparams.mfx.FrameInfo.AspectRatioH;

    int idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION2);
    if (idx < 0)
    {
        // by default return 30
        omxparams.nFixedFrameRate = 30;
    }
    else
    {
        omxparams.nFixedFrameRate = mfxparams.ext_buf[idx].opt2.FixedFrameRate == MFX_CODINGOPTION_OFF ? 0 : 1;
    }

    idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_VIDEO_SIGNAL_INFO);
    if (idx < 0)
    {
        omxparams.nVideoFormat = 0;
        omxparams.nColourDescriptionPresent = 0;
        omxparams.nColourPrimaries = 0;
        omxparams.bVideoFullRange = OMX_FALSE;
    }
    else
    {
        omxparams.nVideoFormat = mfxparams.ext_buf[idx].vsi.VideoFormat;
        omxparams.nColourDescriptionPresent = mfxparams.ext_buf[idx].vsi.ColourDescriptionPresent;
        omxparams.nColourPrimaries = mfxparams.ext_buf[idx].vsi.ColourPrimaries;
        omxparams.bVideoFullRange = mfxparams.ext_buf[idx].vsi.VideoFullRange == 0 ? OMX_FALSE : OMX_TRUE;
    }

    idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION);
    if (idx < 0)
    {
        omxparams.nPicStructPresent = 0;
    }
    else
    {
        omxparams.nPicStructPresent = mfxparams.ext_buf[idx].opt.PicTimingSEI == MFX_CODINGOPTION_OFF ? 0 : 1;
    }

    idx = mfxparams.getExtParamIdx(MFX_EXTBUFF_CODING_OPTION3);
    if (idx < 0)
    {
        omxparams.nLowDelayHRD = 0;
    }
    else
    {
        omxparams.nLowDelayHRD = mfxparams.ext_buf[idx].opt3.LowDelayHrd == MFX_CODINGOPTION_OFF ? 0 : 1;
    }

    MFX_OMX_AT__OMX_VIDEO_ENCODER_VUI_CONTROL(omxparams);
    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT& omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AT__OMX_VIDEO_TEMPORAL_LAYER_COUNT(omxparams);

    int idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_AVC_TEMPORAL_LAYERS);
    if (idx < 0 || idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM) return OMX_ErrorInsufficientResources;

    // first layer is the base one, it must have scale 1 by definition
    config.mfxparams->ext_buf[idx].tempLayers.Layer[0].Scale = 1;
    // next layers have to have increasing scales: 2, 4, 8, etc
    for (mfxU32 i = 1; i < omxparams.nTemproalLayerCount; i++)
    {
        config.mfxparams->ext_buf[idx].tempLayers.Layer[i].Scale =
            TEMPORAL_LAYERS_SCALE_RATIO * config.mfxparams->ext_buf[idx].tempLayers.Layer[i-1].Scale;
    }

    if (config.bCodecInitialized)
    {
        idx = config.mfxparams->enableExtParam(MFX_EXTBUFF_ENCODER_RESET_OPTION);
        if (idx < 0 || idx >= MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM)
            return OMX_ErrorInsufficientResources;
        else
            config.mfxparams->ext_buf[idx].reset.StartNewSequence = MFX_CODINGOPTION_UNKNOWN;
    }

    return OMX_ErrorNone;
}

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE omx2mfx_control(
    MfxOmxInputConfig& config,
    const MfxOmxInputConfig& /*curconfig*/,
    const T* omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (!omxparams) return OMX_ErrorBadParameter;

    config.control = CreateEncodeCtrlWrapper();
    if (!config.control) return OMX_ErrorInsufficientResources;

    OMX_ERRORTYPE omx_res = omx2mfx_config(*config.control, *omxparams);
    if (OMX_ErrorNone != omx_res)
    {
        MFX_OMX_DELETE(config.control);
    }
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#undef DEFINE_TEMPLATE
#define DEFINE_TEMPLATE(_type) \
    template<> \
    OMX_ERRORTYPE omx2mfx_control( \
        MfxOmxInputConfig& config, \
        const MfxOmxInputConfig& curconfig, \
        const _type* params) \
    { \
        config; curconfig; params; \
        return OMX_ErrorNone; \
    }

DEFINE_TEMPLATE(OMX_CONFIG_FRAMERATETYPE)
DEFINE_TEMPLATE(OMX_CONFIG_NUMSLICE)
DEFINE_TEMPLATE(OMX_CONFIG_BITRATELIMITOFF);
DEFINE_TEMPLATE(OMX_CONFIG_NUMREFFRAME);
DEFINE_TEMPLATE(OMX_CONFIG_GOPPICSIZE);
DEFINE_TEMPLATE(OMX_CONFIG_IDRINTERVAL);
DEFINE_TEMPLATE(OMX_CONFIG_GOPREFDIST);
DEFINE_TEMPLATE(OMX_CONFIG_LOWPOWER);
DEFINE_TEMPLATE(OMX_CONFIG_DISABLEDEBLOCKINGIDC);
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_AVCINTRAPERIOD)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_INTEL_BITRATETYPE)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_AVCTYPE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_HEVCTYPE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_HRD_PARAM)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_USERDATA)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_MAX_PICTURE_SIZE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_TARGET_USAGE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_PROFILELEVELTYPE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_BITRATETYPE)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_BITRATETYPE)
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_INTEL_AVCVUI);
DEFINE_TEMPLATE(OMX_VIDEO_PARAM_INTRAREFRESHTYPE)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_OPERATION_RATE)
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_PRIORITY)

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE omx2mfx_params(
    MfxOmxInputConfig& config,
    const MfxOmxInputConfig& curconfig,
    const T* omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (!omxparams) return OMX_ErrorBadParameter;

    config.mfxparams = CreateVideoParamsWrapper(*curconfig.mfxparams, config.bCodecInitialized);
    if (!config.mfxparams) return OMX_ErrorInsufficientResources;

    OMX_ERRORTYPE omx_res = omx2mfx_config(config, *omxparams);
    if (OMX_ErrorNone != omx_res)
    {
        MFX_OMX_DELETE(config.mfxparams);
    }
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#undef DEFINE_TEMPLATE
#define DEFINE_TEMPLATE(_type) \
    template<> \
    OMX_ERRORTYPE omx2mfx_params( \
        MfxOmxInputConfig& config, \
        const MfxOmxInputConfig& curconfig, \
        const _type* params) \
    { \
        config; curconfig; params; \
        return OMX_ErrorNone; \
    }

DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_USERDATA);
DEFINE_TEMPLATE(OMX_VIDEO_CONFIG_DIRTY_RECT);
DEFINE_TEMPLATE(OMX_CONFIG_INTRAREFRESHVOPTYPE);
DEFINE_TEMPLATE(OMX_CONFIG_DUMMYFRAME);

/*------------------------------------------------------------------------------*/

template<typename T>
OMX_ERRORTYPE omx2mfx_config(
    MfxOmxInputConfig& config,
    const MfxOmxInputConfig& curconfig,
    const T* omxparams)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    OMX_ERRORTYPE omx_res = omx2mfx_params(config, curconfig, omxparams);
    if (OMX_ErrorNone != omx_res) goto error;

    omx_res = omx2mfx_control(config, curconfig, omxparams);
    if (OMX_ErrorNone != omx_res) goto error;

    return OMX_ErrorNone;

  error:
    MFX_OMX_DELETE(config.control);
    MFX_OMX_DELETE(config.mfxparams);
    return omx_res;
}

/*------------------------------------------------------------------------------*/

#undef INSTANTIATE
#define INSTANTIATE(_type) \
    template \
    OMX_ERRORTYPE omx2mfx_config( \
        MfxOmxInputConfig& config, \
        const MfxOmxInputConfig& curconfig, \
        const _type* params);

INSTANTIATE(OMX_CONFIG_FRAMERATETYPE);
INSTANTIATE(OMX_CONFIG_INTRAREFRESHVOPTYPE);
INSTANTIATE(OMX_CONFIG_DUMMYFRAME);
INSTANTIATE(OMX_CONFIG_NUMSLICE);
INSTANTIATE(OMX_CONFIG_BITRATELIMITOFF);
INSTANTIATE(OMX_CONFIG_NUMREFFRAME);
INSTANTIATE(OMX_CONFIG_GOPPICSIZE);
INSTANTIATE(OMX_CONFIG_IDRINTERVAL);
INSTANTIATE(OMX_CONFIG_GOPREFDIST);
INSTANTIATE(OMX_CONFIG_LOWPOWER);
INSTANTIATE(OMX_CONFIG_DISABLEDEBLOCKINGIDC);
INSTANTIATE(OMX_VIDEO_CONFIG_USERDATA);
INSTANTIATE(OMX_VIDEO_CONFIG_DIRTY_RECT);
INSTANTIATE(OMX_VIDEO_CONFIG_AVCINTRAPERIOD);
INSTANTIATE(OMX_VIDEO_CONFIG_INTEL_BITRATETYPE);
INSTANTIATE(OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS);
INSTANTIATE(OMX_VIDEO_PARAM_AVCTYPE);
INSTANTIATE(OMX_VIDEO_PARAM_HEVCTYPE);
INSTANTIATE(OMX_VIDEO_PARAM_HRD_PARAM);
INSTANTIATE(OMX_VIDEO_PARAM_MAX_PICTURE_SIZE);
INSTANTIATE(OMX_VIDEO_PARAM_TARGET_USAGE);
INSTANTIATE(OMX_VIDEO_PARAM_ENCODE_FRAME_CROPPING_PARAM);
INSTANTIATE(OMX_VIDEO_PARAM_ENCODE_VUI_CONTROL_PARAM);
INSTANTIATE(OMX_VIDEO_PARAM_PROFILELEVELTYPE);
INSTANTIATE(OMX_VIDEO_PARAM_BITRATETYPE);
INSTANTIATE(OMX_VIDEO_CONFIG_BITRATETYPE);
INSTANTIATE(OMX_VIDEO_PARAM_INTEL_AVCVUI);
INSTANTIATE(OMX_VIDEO_PARAM_INTRAREFRESHTYPE);
INSTANTIATE(OMX_VIDEO_CONFIG_ANDROID_INTRAREFRESHTYPE);
INSTANTIATE(OMX_VIDEO_CONFIG_INTEL_TEMPORALLAYERCOUNT);
INSTANTIATE(OMX_VIDEO_CONFIG_OPERATION_RATE)
INSTANTIATE(OMX_VIDEO_CONFIG_PRIORITY)

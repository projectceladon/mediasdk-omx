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

#ifndef __MFX_OMX_TYPES_H__
#define __MFX_OMX_TYPES_H__

#include "mfx_omx_defs.h"

/*------------------------------------------------------------------------------*/
// on Android OMX_VERSION_* are not defined

#ifndef OMX_VERSION_MAJOR
    #define OMX_VERSION_MAJOR 1
#endif
#ifndef OMX_VERSION_MINOR
    #define OMX_VERSION_MINOR 0
#endif
#ifndef OMX_VERSION_REVISION
    #define OMX_VERSION_REVISION 0
#endif
#ifndef OMX_VERSION_STEP
    #define OMX_VERSION_STEP 0
#endif
#ifndef OMX_VERSION
    #define OMX_VERSION (OMX_VERSION_MAJOR | (OMX_VERSION_MINOR << 8) | (OMX_VERSION_REVISION << 16) | (OMX_VERSION_STEP << 24))
#endif

/*------------------------------------------------------------------------------*/

#define MFX_OMX_COPYRIGHT "Copyright(c) 2011-2018 Intel Corporation"

#ifndef MFX_FILE_VERSION
    #define MFX_FILE_VERSION "0.0.0.0"
#endif
#ifndef MFX_PRODUCT_VERSION
    #define MFX_PRODUCT_VERSION "0.0.000.0000"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
int memcpy_s(void *dest, size_t dmax, const void *src, size_t slen);
int strcpy_s(char *dest, size_t dmax, const char *src);
#ifdef __cplusplus
}
#endif /* __cplusplus */


/*------------------------------------------------------------------------------*/

typedef void* mfx_omx_so_handle;

/* Additional OMX color formats. */
enum
{
    OMX_INTEL_COLOR_Format_NV12 = HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL,   // 0x100
    OMX_INTEL_COLOR_Format_P10 = HAL_PIXEL_FORMAT_P010_INTEL,            // 0x110
};

#ifdef MFX_OMX_USE_GRALLOC_1

#define MFX_OMX_MAX_NUM_OF_PLANES 4
typedef struct intel_ufo_buffer_details_t
{
    uint32_t width;       // \see alloc_device_t::alloc
    uint32_t height;      // \see alloc_device_t::alloc
    int format;           // \see alloc_device_t::alloc

    // buffer pitch (in bytes)
    union {
        uint32_t pitchArray[MFX_OMX_MAX_NUM_OF_PLANES]; // pitch for each plane
        uint32_t pitch;                                 // pitch for the first plane
    };

    uint32_t prime;       // prime buffer descriptor
    uint32_t numOfPlanes; // number of plane for color format
    uint32_t allocWidth;  // allocated buffer width in pixels.
    uint32_t allocHeight; // allocated buffer height in lines.
} intel_ufo_buffer_details_t;
#endif

/* NOTE: at the moment there is no intersections with MFX defined memtypes... */
enum
{
    MFX_OMX_MEMTYPE_GRALLOC = 0x8000
};

enum
{
    MFX_OMX_VIDEO_CodingHEVC = OMX_VIDEO_CodingHEVC
};

enum
{
    MFX_OMX_VIDEO_HEVCProfileMain    = OMX_VIDEO_HEVCProfileMain
};

enum
{
    MFX_OMX_VIDEO_HEVCProfileMain10    = OMX_VIDEO_HEVCProfileMain10
};

#ifdef HEVC10HDR_SUPPORT
enum
{
    MFX_OMX_VIDEO_HEVCProfileMain10HDR10 = OMX_VIDEO_HEVCProfileMain10HDR10
};
#endif

enum
{
    MFX_OMX_VIDEO_HEVCMainTierLevel51 = OMX_VIDEO_HEVCMainTierLevel51
};

enum
{
    MFX_OMX_IndexConfigPriority = OMX_IndexConfigPriority
};

enum
{
    MFX_OMX_IndexConfigOperatingRate = OMX_IndexConfigOperatingRate
};

enum
{
    MFX_OMX_IndexParamConsumerUsageBits = OMX_IndexParamConsumerUsageBits
};

enum
{
    MFX_BITSTREAM_SPSPPS = 0x8000
};

struct OMX_VIDEO_CONFIG_OPERATION_RATE: public OMX_PARAM_U32TYPE {};
struct OMX_VIDEO_CONFIG_PRIORITY: public OMX_PARAM_U32TYPE {};

enum
{
    MFX_OMX_PRIORITY_REALTIME = 0,
    MFX_OMX_PRIORITY_PERFORMANCE = 1
};

/*------------------------------------------------------------------------------*/

enum MfxOmxBufferType
{
    MfxOmxBuffer_Bitstream,
    MfxOmxBuffer_Surface
};

/*------------------------------------------------------------------------------*/

union MfxOmxExtBuffer
{
    mfxExtBuffer header;
    mfxExtCodingOption opt;
    mfxExtCodingOption2 opt2;
    mfxExtCodingOption3 opt3;
    mfxExtEncoderResetOption reset;
    mfxExtAvcTemporalLayers tempLayers;
    mfxExtVideoSignalInfo vsi;
    mfxExtEncoderROI roi;
};

/*------------------------------------------------------------------------------*/

#define MFX_OMX_SEI_USER_DATA_UNREGISTERED_TYPE 5

/*------------------------------------------------------------------------------*/

#define MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM 9
#define MFX_OMX_ENCODE_CTRL_EXTBUF_MAX_NUM 9

#define MFX_OMX_ENCODE_EXTBUF_MAX_NUM MFX_OMX_MAX(MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM, MFX_OMX_ENCODE_CTRL_EXTBUF_MAX_NUM)

template<typename T, size_t N>
struct MfxOmxParamsWrapper: public T
{
    mfxExtBuffer* ext_buf_ptrs[N];
    MfxOmxExtBuffer ext_buf[N];
    mfxPayload* payload;
    struct
    {
        bool enabled;
        int idx;
    } ext_buf_idxmap[MFX_OMX_ENCODE_EXTBUF_MAX_NUM];

    MfxOmxParamsWrapper()
    {
        MFX_OMX_AUTO_TRACE_FUNC();

        memset(static_cast<T*>(this), 0, sizeof(T));
        payload = NULL;
        if (!N) return;

        MFX_OMX_ZERO_MEMORY(ext_buf_ptrs);
        MFX_OMX_ZERO_MEMORY(ext_buf);
        MFX_OMX_ZERO_MEMORY(ext_buf_idxmap);

        this->ExtParam = ext_buf_ptrs;
        for (size_t i = 0; i < N; ++i)
        {
            ext_buf_ptrs[i] = (mfxExtBuffer*)&ext_buf[i];
        }
    }
    MfxOmxParamsWrapper(const MfxOmxParamsWrapper& ref)
    {
        *this = ref; // call to operator=
    }
    MfxOmxParamsWrapper& operator=(const MfxOmxParamsWrapper& ref)
    {
        T* dst = this;
        const T* src = &ref;

        memcpy_s(dst, sizeof(T), src, sizeof(T));
        payload = ref.payload;
        if (!N) return *this;

        MFX_OMX_ZERO_MEMORY(ext_buf_ptrs);
        MFX_OMX_COPY(ext_buf, ref.ext_buf);
        MFX_OMX_COPY(ext_buf_idxmap, ref.ext_buf_idxmap);

        this->ExtParam = ext_buf_ptrs;
        for (size_t i = 0; i < N; ++i)
        {
            ext_buf_ptrs[i] = (mfxExtBuffer*)&ext_buf[i];
        }
        return *this;
    }
    MfxOmxParamsWrapper(const T& ref)
    {
        *this = ref; // call to operator=
    }
    MfxOmxParamsWrapper& operator=(const T& ref)
    {
        T* dst = this;
        const T* src = &ref;

        memcpy_s(dst, sizeof(T), src, sizeof(T));
        payload = NULL;
        if (!N) return *this;

        MFX_OMX_ZERO_MEMORY(ext_buf_ptrs);
        MFX_OMX_ZERO_MEMORY(ext_buf);
        MFX_OMX_ZERO_MEMORY(ext_buf_idxmap);

        this->ExtParam = ext_buf_ptrs;
        for (size_t i = 0; i < N; ++i)
        {
            ext_buf_ptrs[i] = (mfxExtBuffer*)&ext_buf[i];
        }
        return *this;
    }
    void ResetExtParams()
    {
        this->NumExtParam = 0;
        this->ExtParam = (N)? ext_buf_ptrs: NULL;
    }
    /** Function returns index of the already enabled buffer */
    int getExtParamIdx(mfxU32 bufferid) const
    {
        MFX_OMX_AUTO_TRACE_FUNC();

        int idx_map = getEnabledMapIdx(bufferid);

        MFX_OMX_AUTO_TRACE_I32(idx_map);
        if (idx_map < 0) return -1;

        MFX_OMX_AUTO_TRACE_I32(ext_buf_idxmap[idx_map].enabled);
        MFX_OMX_AUTO_TRACE_I32(ext_buf_idxmap[idx_map].idx);
        if (ext_buf_idxmap[idx_map].enabled) return ext_buf_idxmap[idx_map].idx;
        return -1;
    }
    /** Function returns index of the enabled buffer
     * and enables it if it is not yet enabled.
     */
    int enableExtParam(mfxU32 bufferid)
    {
        MFX_OMX_AUTO_TRACE_FUNC();

        int idx_map = getEnabledMapIdx(bufferid);

        MFX_OMX_AUTO_TRACE_I32(idx_map);
        if (idx_map < 0) return -1;

        MFX_OMX_AUTO_TRACE_I32(ext_buf_idxmap[idx_map].enabled);
        MFX_OMX_AUTO_TRACE_I32(ext_buf_idxmap[idx_map].idx);
        if (ext_buf_idxmap[idx_map].enabled) return ext_buf_idxmap[idx_map].idx;

        if (this->NumExtParam >= N)
        {
            MFX_OMX_AUTO_TRACE_MSG("too many parameters to hold");
            return -1;
        }

        int idx = this->NumExtParam++;

        MFX_OMX_AUTO_TRACE_I32(this->NumExtParam);
        MFX_OMX_AUTO_TRACE_I32(idx);
        ext_buf_idxmap[idx_map].enabled = true;
        ext_buf_idxmap[idx_map].idx = idx;

        MFX_OMX_ZERO_MEMORY(ext_buf[idx]);
        switch (bufferid)
        {
          case MFX_EXTBUFF_CODING_OPTION:
            ext_buf[idx].opt.Header.BufferId = MFX_EXTBUFF_CODING_OPTION;
            ext_buf[idx].opt.Header.BufferSz = sizeof(mfxExtCodingOption);
            return idx;
          case MFX_EXTBUFF_CODING_OPTION2:
            ext_buf[idx].opt2.Header.BufferId = MFX_EXTBUFF_CODING_OPTION2;
            ext_buf[idx].opt2.Header.BufferSz = sizeof(mfxExtCodingOption2);
            return idx;
          case MFX_EXTBUFF_CODING_OPTION3:
            ext_buf[idx].opt3.Header.BufferId = MFX_EXTBUFF_CODING_OPTION3;
            ext_buf[idx].opt3.Header.BufferSz = sizeof(mfxExtCodingOption3);
            return idx;
          case MFX_EXTBUFF_ENCODER_RESET_OPTION:
            ext_buf[idx].reset.Header.BufferId = MFX_EXTBUFF_ENCODER_RESET_OPTION;
            ext_buf[idx].reset.Header.BufferSz = sizeof(mfxExtEncoderResetOption);
            return idx;
          case MFX_EXTBUFF_AVC_TEMPORAL_LAYERS:
            ext_buf[idx].reset.Header.BufferId = MFX_EXTBUFF_AVC_TEMPORAL_LAYERS;
            ext_buf[idx].reset.Header.BufferSz = sizeof(mfxExtAvcTemporalLayers);
            return idx;
          case MFX_EXTBUFF_VIDEO_SIGNAL_INFO:
            ext_buf[idx].reset.Header.BufferId = MFX_EXTBUFF_VIDEO_SIGNAL_INFO;
            ext_buf[idx].reset.Header.BufferSz = sizeof(mfxExtVideoSignalInfo);
            return idx;
          case MFX_EXTBUFF_ENCODER_ROI:
            ext_buf[idx].roi.Header.BufferId = MFX_EXTBUFF_ENCODER_ROI;
            ext_buf[idx].roi.Header.BufferSz = sizeof(mfxExtEncoderROI);
            return idx;
          default:
            // if we are here - that's a bug: add index to getEnabledMapIdx()
            return -1;
        };
    }

protected:
    int getEnabledMapIdx(mfxU32 bufferid) const
    {
        int idx = 0;

        if (!N) return -1;
        switch (bufferid)
        {
          case MFX_EXTBUFF_ENCODER_ROI:
            ++idx;
          case MFX_EXTBUFF_VIDEO_SIGNAL_INFO:
            ++idx;
          case MFX_EXTBUFF_AVC_TEMPORAL_LAYERS:
            ++idx;
          case MFX_EXTBUFF_ENCODER_RESET_OPTION:
            ++idx;
          case MFX_EXTBUFF_CODING_OPTION3:
            ++idx;
          case MFX_EXTBUFF_CODING_OPTION2:
            ++idx;
          case MFX_EXTBUFF_CODING_OPTION:
            if (idx >= MFX_OMX_ENCODE_EXTBUF_MAX_NUM) return -1;
            return idx;
          default:
            return -1;
        };
    }
};

typedef
    MfxOmxParamsWrapper<mfxVideoParam, MFX_OMX_ENCODE_VIDEOPARAM_EXTBUF_MAX_NUM>
    MfxOmxVideoParamsWrapper;

typedef
    MfxOmxParamsWrapper<mfxEncodeCtrl, MFX_OMX_ENCODE_CTRL_EXTBUF_MAX_NUM>
    MfxOmxEncodeCtrlWrapper;

/*------------------------------------------------------------------------------*/

struct MfxOmxInputConfig
{
    bool bCodecInitialized;
    MfxOmxVideoParamsWrapper* mfxparams;
    MfxOmxEncodeCtrlWrapper* control;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxBufferInfo
{
    bool bSelfAllocatedBuffer;
    MfxOmxBufferType eType;
    union
    {
        struct
        {
            mfxFrameSurface1 sSurface;
            MfxOmxInputConfig config;
            OMX_U32 nBufferIndex;
            ANativeWindowBuffer* pAnwBuffer;
            bool bUsed;
        };
        mfxBitstream sBitstream;
    };
    mfxSyncPoint* pSyncPoint;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxBufferSwapPair
{
    MfxOmxBufferInfo*     pAddBufInfoFrom;
    MfxOmxBufferInfo*     pAddBufInfoTo;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxVideoErrorBuffer : public OMX_VIDEO_ERROR_BUFFER
{
    OMX_U32 index;
};

/*------------------------------------------------------------------------------*/

struct MfxOmxProfileLevelTable
{
    OMX_U32 profile;
    OMX_U32 level;
};

/*------------------------------------------------------------------------------*/

struct MFX_OMX_PARAM_PORTINFOTYPE
{
    OMX_U32 nPortIndex;
    OMX_U32 nCurrentBuffersCount;
    bool bEnable;
    bool bDisable;
};

/*------------------------------------------------------------------------------*/
// function types for Open MAX IL Core

typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_Init_Func)(void);
typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_Deinit_Func)(void);
typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_ComponentNameEnum_Func)(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex);
typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_GetHandle_Func)(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN  OMX_STRING cComponentName,
    OMX_IN  OMX_PTR pAppData,
    OMX_IN  OMX_CALLBACKTYPE* pCallBacks);
typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_FreeHandle_Func)(
    OMX_IN  OMX_HANDLETYPE hComponent);
typedef OMX_ERRORTYPE OMX_APIENTRY (*MFX_OMX_SetupTunnel_Func)(
    OMX_IN  OMX_HANDLETYPE hOutput,
    OMX_IN  OMX_U32 nPortOutput,
    OMX_IN  OMX_HANDLETYPE hInput,
    OMX_IN  OMX_U32 nPortInput);
typedef OMX_ERRORTYPE (*MFX_OMX_GetContentPipe_Func)(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI);
typedef OMX_ERRORTYPE (*MFX_OMX_GetComponentsOfRole_Func)(
    OMX_IN      OMX_STRING role,
    OMX_INOUT   OMX_U32 *pNumComps,
    OMX_INOUT   OMX_U8  **compNames);
typedef OMX_ERRORTYPE (*MFX_OMX_GetRolesOfComponent_Func)(
    OMX_IN      OMX_STRING compName,
    OMX_INOUT   OMX_U32 *pNumRoles,
    OMX_OUT     OMX_U8 **roles);

#define MFX_OMX_CORE_INIT_FUNC "OMX_Init"
#define MFX_OMX_CORE_DEINIT_FUNC "OMX_Deinit"
#define MFX_OMX_CORE_COMPONENT_NAME_ENUM_FUNC "OMX_ComponentNameEnum"
#define MFX_OMX_CORE_GET_HANDLE_FUNC "OMX_GetHandle"
#define MFX_OMX_CORE_FREE_HANDLE_FUNC "OMX_FreeHandle"
#define MFX_OMX_CORE_SETUP_TUNNEL_FUNC "OMX_SetupTunnel"
#define MFX_OMX_CORE_GET_CONTENT_PIPE_FUNC "OMX_GetContentPipe"
#define MFX_OMX_CORE_GET_COMPONENTS_OF_ROLE_FUNC "OMX_GetComponentsOfRole"
#define MFX_OMX_CORE_GET_ROLES_OF_COMPONENT_FUNC "OMX_GetRolesOfComponent"

/*------------------------------------------------------------------------------*/
// function types for Open MAX IL Components

enum
{
    MFX_OMX_COMPONENT_FLAGS_NONE = 0x0,
    MFX_OMX_COMPONENT_FLAGS_DUMP_INPUT = 0x01,
    MFX_OMX_COMPONENT_FLAGS_DUMP_OUTPUT = 0x02,
};

// implementation specific functions
typedef OMX_ERRORTYPE (*MFX_OMX_ComponentInit_Func)(
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_U32 nFlags,
    OMX_IN OMX_BOOL bCreateComponent,
    OMX_INOUT OMX_HANDLETYPE hComponent);

#define MFX_OMX_COMPONENT_INIT_FUNC "MFX_OMX_ComponentInit"

typedef enum
{
    MfxMetadataBufferTypeCameraSource = 0, // android::kMetadataBufferTypeCameraSource
    MfxMetadataBufferTypeGrallocSource = 1, // android::kMetadataBufferTypeGrallocSource
    MfxMetadataBufferTypeANWBuffer = 2, // android::kMetadataBufferTypeANWBuffer
    MfxMetadataBufferTypeNativeHandleSource = 3, //android::kMetadataBufferTypeNativeHandleSource
    MfxMetadataBufferTypeInvalid = -1, //android::kMetadataBufferTypeInvalid
} MfxMetadataBufferType;

struct MetadataBuffer
{
    MfxMetadataBufferType type;
    ANativeWindowBuffer* anw_buffer;
    buffer_handle_t handle;
    mfxI32 * pFenceFd;
};

/*------------------------------------------------------------------------------*/

enum
{
#ifdef INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_TIMESTAMP
    MFX_INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_TIMESTAMP = INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_TIMESTAMP,
#else
    MFX_INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_TIMESTAMP = 30,
#endif

#ifdef INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_FPS
    MFX_INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_FPS = INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_FPS,
#else
    MFX_INTEL_UFO_GRALLOC_MODULE_PERFORM_SET_BO_FPS = 35,
#endif
};

/*------------------------------------------------------------------------------*/

enum eMfxOmxHwType
{
    MFX_HW_UNKNOWN = 0,
    MFX_HW_BXT
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_TYPES_H__

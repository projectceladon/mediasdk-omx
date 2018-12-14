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

#ifndef __MFX_OMX_DEFS_H__
#define __MFX_OMX_DEFS_H__

// OMX Headers
#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OMX_IndexExt.h>
#include <OMX_VideoExt.h>
#include <../../openmax/intel/OMX_IntelVideoExt.h>
#include <../../openmax/intel/OMX_IntelIndexExt.h>

#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

#ifdef LIBVA_SUPPORT
    #include <va/va.h>
#if (MFX_ANDROID_VERSION < MFX_O_MR1)
    #include <va/va_tpi.h>
#endif
#endif

#include <android/log.h>
#include <utils/Mutex.h>
#include <utils/Trace.h>
#include <cutils/native_handle.h>

#if (MFX_ANDROID_VERSION >= MFX_O_MR1)
    #include <nativebase/nativebase.h>
#endif

#include <media/hardware/HardwareAPI.h>
#include <hardware/gralloc.h>
#include <ui/Fence.h>

#ifdef MFX_OMX_USE_GRALLOC_1
    #define DRV_I915
    #define USE_GRALLOC1
    #include <i915_private_android_types.h>
    #include <cros_gralloc_types.h>
#else
    #include <ufo/gralloc.h>
    #include <ufo/graphics.h>
#endif

#include <media/hardware/MetadataBufferType.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// MFX headers
#include <mfxdefs.h>
#include <mfxstructures.h>
#include <mfxvideo.h>
#include <mfxvideo++.h>
#include <mfxvp8.h>
#include <mfxvp9.h>
#include <mfx_android_config.h>

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#define MFX_OMX_NO  0
#define MFX_OMX_YES 1

/* Notes:
 *  - MFX_OMX_DEBUG - will switch on massive text logging
 *  - MFX_OMX_DEBUG_DUMP - will switch on input/output file dumps (need configure
 *  additionally thru the plug-ins registry file)
 */
#define MFX_OMX_DEBUG MFX_OMX_NO
#define MFX_OMX_DEBUG_DUMP MFX_OMX_NO
#define MFX_OMX_PERF MFX_OMX_NO

//#define MFX_OMX_STDOUT MFX_OMX_NO
#define MFX_OMX_LOG_TAG "mediasdk_omx"
#define MFX_OMX_LOG_LEVEL ANDROID_LOG_DEBUG

#ifndef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "unknown"
#endif

/*------------------------------------------------------------------------------*/

#define gmin         1 //CHT
#define broxton      2
#define kabylake     3
#define cannonlake   4
#define icelakeu     5
#define tigerlake    6
#define elkhartlake  7

/*------------------------------------------------------------------------------*/

#define MFX_OMX_MAX_PATH 260
#define MFX_OMX_ROUGH_VERSION_CHECK MFX_OMX_NO
#define MFX_OMX_VALID_COMPONENT_PREFIX "OMX.Intel."
#define MFX_OMX_CONFIG_FILE_NAME "mfx_omxil_core.conf"
#define MFX_OMX_CONFIG_FILE_PATH "/vendor/etc"
#define MFX_OMX_COMPONENT_LIMIT_FILE_NAME "mfx_omxil_resource_limitation.xml"
#ifndef MFX_OMX_VERSION
    #define MFX_OMX_VERSION 0
#endif
#define MFX_OMX_TIME_STAMP_FREQ 90000
#define MFX_OMX_IMPLEMENTATION MFX_IMPL_AUTO | MFX_IMPL_VIA_ANY
#define MFX_OMX_INFINITE 0xEFFFFFFF
#define MFX_OMX_TIME_STAMP_INVALID ((mfxU64)-1.0)

/*------------------------------------------------------------------------------*/

#define MFX_OMX_CLASS_NO_COPY(_class) \
    _class(const _class&); \
    _class& operator=(const _class&);

#define MFX_OMX_NEW(_ptr, _class) \
    { try { (_ptr) = new _class; } catch(...) { (_ptr) = NULL; } }

#define MFX_OMX_THROW_IF(_if, _what) \
    { if (_if) throw _what; }

#define MFX_OMX_TRY_AND_CATCH(_try, _catch) \
    { try { _try; } catch(...) { _catch; } }

#define MFX_OMX_DELETE(_ptr) \
    { if (_ptr) { delete (_ptr); (_ptr) = NULL; } }

#define MFX_OMX_FREE(_ptr) \
    { if (_ptr) { free(_ptr); (_ptr) = NULL; } }

#define MFX_OMX_ZERO_MEMORY(_obj) \
    { memset(&(_obj), 0, sizeof(_obj)); }

#define MFX_OMX_COPY(_dst, _src) \
    { memcpy_s(&(_dst), sizeof(_dst), &(_src), sizeof(_src)); }

#define MFX_OMX_MAX(A, B) (((A) > (B)) ? (A) : (B))

#define MFX_OMX_MIN(A, B) (((A) < (B)) ? (A) : (B))

#define MFX2OMX_TIME(_mfx_time) \
    (OMX_TICKS)(((mfxF64)(_mfx_time) / (mfxF64)MFX_OMX_TIME_STAMP_FREQ) * OMX_TICKS_PER_SECOND)

#define OMX2MFX_TIME(_omx_time) \
    (mfxU64)(((mfxF64)(_omx_time) / OMX_TICKS_PER_SECOND) * MFX_OMX_TIME_STAMP_FREQ)

#define SEC2OMX_TIME(_sec_time) \
    (OMX_TICKS)((_sec_time)*(mfxF64)OMX_TICKS_PER_SECOND)

#define OMX2SEC_TIME(_omx_time) \
    ((mfxF64)(_omx_time)/(mfxF64)OMX_TICKS_PER_SECOND)

#define MFX_OMX_MEM_ALIGN(X, N) ((X) & ((N)-1)) ? (((X)+(N)-1) & (~((N)-1))): (X)

#define MFX_OMX_GET_ARRAY_SIZE(_array) \
    sizeof(_array)/sizeof(_array[0])

#define MFX_OMX_UNUSED(x) (void)(x);

/*------------------------------------------------------------------------------*/
/* WorkAround
** We need to reestablish exact input PTS on output port for pass some CTS test.
** But convertation PTS to mfx format doesn't allow it. So we just sent clear PTS
** direct to msdk.
*/
#if 1
    #undef  OMX2MFX_TIME
    #define OMX2MFX_TIME(_omx_time) (mfxU64)(_omx_time)

    #undef  MFX2OMX_TIME
    #define MFX2OMX_TIME(_mfx_time) (OMX_TICKS)(_mfx_time)
#endif

/*------------------------------------------------------------------------------*/

#define MFX_OMX_SET_STRUCT_VERSION(T) \
    sizeof(T), \
    { \
      .s = { \
            .nVersionMajor = OMX_VERSION_MAJOR, \
            .nVersionMinor = OMX_VERSION_MINOR, \
            .nRevision = OMX_VERSION_REVISION, \
            .nStep = OMX_VERSION_STEP \
           } \
    }

/*------------------------------------------------------------------------------*/

#if MFX_OMX_DEBUG == MFX_OMX_YES

    #if defined(MFX_OMX_STDOUT)
        #ifdef MFX_OMX_FILE_INIT
            #if MFX_OMX_STDOUT == MFX_OMX_YES
                FILE* g_dbg_file = stdout;
            #else // // #if MFX_OMX_STDOUT == MFX_OMX_YES
                #ifndef MFX_OMX_FILE_NAME
                    #define MFX_OMX_FILE_NAME "/data/local/tmp/mfx_omx_log.txt"
                #endif // #ifndef MFX_OMX_FILE_NAME
                FILE* g_dbg_file = fopen(MFX_OMX_FILE_NAME, "a");
            #endif // #if MFX_OMX_STDOUT == MFX_OMX_YES
        #else // #ifdef MFX_OMX_FILE_INIT
            extern FILE* g_dbg_file;
        #endif // #ifdef MFX_OMX_FILE_INIT
    #endif

    class mfx_omx_trace
    {
    public:
        mfx_omx_trace(const char* _modulename, const char* _function, const char* _taskname);
        ~mfx_omx_trace(void);
        void printf_msg(const char* msg);
        void printf_i32(const char* name, mfxI32 value);
        void printf_u32(const char* name, mfxU32 value);
        void printf_i64(const char* name, mfxI64 value);
        void printf_f64(const char* name, mfxF64 value);
        void printf_p(const char* name, void* value);
        void printf_s(const char* name, const char* value);

    protected:
        const char* modulename;
        const char* function;
        const char* taskname;
    private:
        MFX_OMX_CLASS_NO_COPY(mfx_omx_trace)
    };

    #define MFX_OMX_AUTO_TRACE(_task_name) \
        mfx_omx_trace _mfx_omx_trace(MFX_OMX_MODULE_NAME, __FUNCTION__, _task_name)

    #define MFX_OMX_AUTO_TRACE_FUNC() \
        MFX_OMX_AUTO_TRACE(NULL)

    #define MFX_OMX_AUTO_TRACE_MSG(_arg) \
        _mfx_omx_trace.printf_msg(_arg)

    #define MFX_OMX_AUTO_TRACE_I32(_arg) \
        _mfx_omx_trace.printf_i32(#_arg, (mfxI32)_arg)

    #define MFX_OMX_AUTO_TRACE_U32(_arg) \
        _mfx_omx_trace.printf_u32(#_arg, (mfxU32)_arg)

    #define MFX_OMX_AUTO_TRACE_I64(_arg) \
        _mfx_omx_trace.printf_i64(#_arg, (mfxI64)_arg)

    #define MFX_OMX_AUTO_TRACE_F64(_arg) \
        _mfx_omx_trace.printf_f64(#_arg, (mfxF64)_arg)

    #define MFX_OMX_AUTO_TRACE_P(_arg) \
        _mfx_omx_trace.printf_p(#_arg, (void*)_arg)

    #define MFX_OMX_AUTO_TRACE_S(_arg) \
        _mfx_omx_trace.printf_s(#_arg, _arg)

    #define MFX_OMX_PRINT_LOG(...) \
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, __VA_ARGS__)

    #define MFX_OMX_LOG(_arg, ...) \
        MFX_OMX_PRINT_LOG("%s: %s: " _arg, MFX_OMX_MODULE_NAME, __FUNCTION__, ##__VA_ARGS__)

#else // #if MFX_OMX_DEBUG == MFX_OMX_YES

    #define MFX_OMX_AUTO_TRACE(_task_name)
    #define MFX_OMX_AUTO_TRACE_FUNC()

    #define MFX_OMX_AUTO_TRACE_MSG(_arg)
    #define MFX_OMX_AUTO_TRACE_I32(_arg)
    #define MFX_OMX_AUTO_TRACE_U32(_arg)
    #define MFX_OMX_AUTO_TRACE_I64(_arg)
    #define MFX_OMX_AUTO_TRACE_F64(_arg)
    #define MFX_OMX_AUTO_TRACE_P(_arg)
    #define MFX_OMX_AUTO_TRACE_S(_arg)
    #define MFX_OMX_LOG(_arg, ...)

#endif // #if MFX_OMX_DEBUG == MFX_OMX_YES

#define MFX_OMX_PRINT_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, MFX_OMX_LOG_TAG, __VA_ARGS__)

#define MFX_OMX_LOG_ERROR(_arg, ...) \
    MFX_OMX_PRINT_ERROR("%s: %s[line %d]: " _arg, MFX_OMX_MODULE_NAME, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define MFX_OMX_PRINT_INFO(...) \
    __android_log_print(ANDROID_LOG_INFO, MFX_OMX_LOG_TAG, __VA_ARGS__)

#define MFX_OMX_LOG_INFO(_arg, ...) \
    MFX_OMX_PRINT_INFO("%s: " _arg, MFX_OMX_MODULE_NAME, ##__VA_ARGS__)

#define MFX_OMX_CONDITION(condition)     (__builtin_expect((condition)!=0, 0))
#define MFX_OMX_LOG_INFO_IF(condition, _arg, ...) \
    ( (MFX_OMX_CONDITION(condition)) \
    ? ((void)MFX_OMX_PRINT_INFO("%s: " _arg, MFX_OMX_MODULE_NAME, ##__VA_ARGS__)) \
    : (void)0 )

const char* mfx_omx_code_to_string(mfxStatus sts);

/*------------------------------------------------------------------------------*/

#if MFX_OMX_PERF == MFX_OMX_YES
    class mfx_omx_perf
    {
    public:
        mfx_omx_perf(const char* _modulename, const char* _function, const char* _taskname);
        ~mfx_omx_perf(void);

    protected:
        const char* modulename;
        const char* function;
        const char* taskname;
        unsigned long long int start;
    private:
        MFX_OMX_CLASS_NO_COPY(mfx_omx_perf)
    };

    #define MFX_OMX_AUTO_PERF(_task_name) \
        mfx_omx_perf _mfx_omx_perf(MFX_OMX_MODULE_NAME, __FUNCTION__, _task_name)

    #define MFX_OMX_AUTO_PERF_FUNC() \
        MFX_OMX_AUTO_PERF(NULL)

#else // #if MFX_OMX_DEBUG == MFX_OMX_YES

    #define MFX_OMX_AUTO_PERF(_task_name)
    #define MFX_OMX_AUTO_PERF_FUNC()

#endif // #if MFX_OMX_PERF == MFX_OMX_YES

/*------------------------------------------------------------------------------*/

// NOTE: AT - AUTO_TRACE
#define MFX_OMX_AT__OMX_STRUCT(_struct) \
    MFX_OMX_AUTO_TRACE_I32(_struct.nSize); \
    MFX_OMX_AUTO_TRACE_U32(_struct.nVersion.nVersion);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PORTDEFINITIONTYPE(_vport) \
    MFX_OMX_AUTO_TRACE_S(_vport.cMIMEType); \
    MFX_OMX_AUTO_TRACE_P(_vport.pNativeRender); \
    MFX_OMX_AUTO_TRACE_I32(_vport.nFrameWidth); \
    MFX_OMX_AUTO_TRACE_I32(_vport.nFrameHeight); \
    MFX_OMX_AUTO_TRACE_I32(_vport.nStride); \
    MFX_OMX_AUTO_TRACE_I32(_vport.nSliceHeight); \
    MFX_OMX_AUTO_TRACE_I32(_vport.nBitrate); \
    MFX_OMX_AUTO_TRACE_U32(_vport.xFramerate); \
    MFX_OMX_AUTO_TRACE_I32(_vport.bFlagErrorConcealment); \
    MFX_OMX_AUTO_TRACE_U32(_vport.eCompressionFormat); \
    MFX_OMX_AUTO_TRACE_U32(_vport.eColorFormat); \
    MFX_OMX_AUTO_TRACE_P(_vport.pNativeWindow);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_PARAM_PORTDEFINITIONTYPE(_port) \
    MFX_OMX_AT__OMX_STRUCT(_port); \
    MFX_OMX_AUTO_TRACE_I32(_port.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_port.eDir); \
    MFX_OMX_AUTO_TRACE_I32(_port.nBufferCountActual); \
    MFX_OMX_AUTO_TRACE_I32(_port.nBufferCountMin); \
    MFX_OMX_AUTO_TRACE_I32(_port.nBufferSize); \
    MFX_OMX_AUTO_TRACE_I32(_port.bEnabled); \
    MFX_OMX_AUTO_TRACE_I32(_port.bPopulated); \
    MFX_OMX_AUTO_TRACE_I32(_port.eDomain); \
    MFX_OMX_AT__OMX_VIDEO_PORTDEFINITIONTYPE(_port.format.video); \
    MFX_OMX_AUTO_TRACE_I32(_port.bBuffersContiguous); \
    MFX_OMX_AUTO_TRACE_I32(_port.nBufferAlignment);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_BITRATETYPE(_bitratetype) \
    MFX_OMX_AT__OMX_STRUCT(_bitratetype); \
    MFX_OMX_AUTO_TRACE_I32(_bitratetype.nPortIndex); \
    MFX_OMX_AUTO_TRACE_U32(_bitratetype.eControlRate); \
    MFX_OMX_AUTO_TRACE_I32(_bitratetype.nTargetBitrate);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_BITRATETYPE(_bitratetype) \
    MFX_OMX_AT__OMX_STRUCT(_bitratetype); \
    MFX_OMX_AUTO_TRACE_I32(_bitratetype.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_bitratetype.nEncodeBitrate);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_FRAMERATETYPE(_param) \
    MFX_OMX_AT__OMX_STRUCT(_param); \
    MFX_OMX_AUTO_TRACE_I32(_param.nPortIndex); \
    MFX_OMX_AUTO_TRACE_U32(_param.xEncodeFramerate);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_BITRATETYPE(_config_intel_bitrate_type) \
    MFX_OMX_AT__OMX_STRUCT(_config_intel_bitrate_type); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nMaxEncodeBitrate); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nTargetPercentage); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nWindowSize); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nInitialQP); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nMinQP); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nMaxQP); \
    MFX_OMX_AUTO_TRACE_U32(_config_intel_bitrate_type.nFrameRate); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_bitrate_type.nTemporalID);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVCVUI(_intel_avcvui) \
    MFX_OMX_AT__OMX_STRUCT(_intel_avcvui); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avcvui.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avcvui.bVuiGeneration);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_HRD_PARAM(_intel_hrd) \
    MFX_OMX_AT__OMX_STRUCT(_intel_hrd); \
    MFX_OMX_AUTO_TRACE_I32(_intel_hrd.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_hrd.nHRDBufferSize); \
    MFX_OMX_AUTO_TRACE_I32(_intel_hrd.nHRDInitialFullness); \
    MFX_OMX_AUTO_TRACE_I32(_intel_hrd.bWriteHRDSyntax);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_MAX_PICTURE_SIZE(_intel_picturesize) \
    MFX_OMX_AT__OMX_STRUCT(_intel_picturesize); \
    MFX_OMX_AUTO_TRACE_I32(_intel_picturesize.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_picturesize.nMaxPictureSize);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_TARGET_USAGE(_intel_targetusage) \
    MFX_OMX_AT__OMX_STRUCT(_intel_targetusage); \
    MFX_OMX_AUTO_TRACE_I32(_intel_targetusage.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_targetusage.nTargetUsage);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_ENCODER_FRAME_CROPPING(_intel_encoderframecropping) \
    MFX_OMX_AT__OMX_STRUCT(_intel_encoderframecropping); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderframecropping.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderframecropping.nCropX); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderframecropping.nCropY); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderframecropping.nCropW); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderframecropping.nCropH);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_ENCODER_VUI_CONTROL(_intel_encodervuicontrol) \
    MFX_OMX_AT__OMX_STRUCT(_intel_encodervuicontrol); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nAspectRatioW); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nAspectRatioH); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nVideoFormat); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nColourDescriptionPresent); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nColourPrimaries); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nFixedFrameRate); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.nPicStructPresent); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encodervuicontrol.bVideoFullRange);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_ENCODER_USER_DATA(_intel_encoderuserdata) \
    MFX_OMX_AT__OMX_STRUCT(_intel_encoderuserdata); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderuserdata.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderuserdata.nUserDataSize); \
    MFX_OMX_AUTO_TRACE_P(_intel_encoderuserdata.pUserData);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_ENCODER_DIRTY_RECT(_intel_encoderdirtyrect) \
    MFX_OMX_AT__OMX_STRUCT(_intel_encoderdirtyrect); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.nNumRectangles); \
    MFX_OMX_AUTO_TRACE_P(_intel_encoderdirtyrect.pRectangles); \
    if (_intel_encoderdirtyrect.pRectangles) \
    { \
            MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.pRectangles[0].nLeft); \
            MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.pRectangles[0].nTop); \
            MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.pRectangles[0].nWidth); \
            MFX_OMX_AUTO_TRACE_I32(_intel_encoderdirtyrect.pRectangles[0].nHeight); \
    };

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_INTRAREFRESHTYPE(_intrarefreshtype) \
    MFX_OMX_AT__OMX_STRUCT(_intrarefreshtype); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshtype.nPortIndex); \
    MFX_OMX_AUTO_TRACE_U32(_intrarefreshtype.eRefreshMode); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshtype.nAirMBs); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshtype.nAirRef); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshtype.nCirMBs);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_INTRAREFRESHVOPTYPE(_intrarefreshVOPtype) \
    MFX_OMX_AT__OMX_STRUCT(_intrarefreshVOPtype); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshVOPtype.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intrarefreshVOPtype.IntraRefreshVOP);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_DUMMYFRAME(_dummyframe) \
    MFX_OMX_AT__OMX_STRUCT(_dummyframe); \
    MFX_OMX_AUTO_TRACE_I32(_dummyframe.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_dummyframe.bInsertDummyFrame);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_NUMSLICE(_numslice) \
    MFX_OMX_AT__OMX_STRUCT(_numslice); \
    MFX_OMX_AUTO_TRACE_I32(_numslice.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_numslice.nSliceNumber);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_INTEL_SLICE_NUMBERS(_config_intel_slice_numbers) \
    MFX_OMX_AT__OMX_STRUCT(_config_intel_slice_numbers); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_slice_numbers.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_slice_numbers.nISliceNumber); \
    MFX_OMX_AUTO_TRACE_I32(_config_intel_slice_numbers.nPSliceNumber);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_BITRATELIMITOFF(_bitrate_limit_off) \
    MFX_OMX_AT__OMX_STRUCT(_bitrate_limit_off); \
    MFX_OMX_AUTO_TRACE_I32(_bitrate_limit_off.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_bitrate_limit_off.bBitrateLimitOff);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_NUMREFFRAME(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.nRefFrame);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_GOPPICSIZE(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.nGopPicSize);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_IDRINTERVAL(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.nIdrInterval);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_GOPREFDIST(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.nGopRefDist);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_LOWPOWER(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.bLowPower);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_CONFIG_DISABLEDEBLOCKINGIDC(_disable_deblocking_idc) \
    MFX_OMX_AT__OMX_STRUCT(_disable_deblocking_idc); \
    MFX_OMX_AUTO_TRACE_I32(_disable_deblocking_idc.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_disable_deblocking_idc.bDisableDeblockingIdc);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_CONFIG_AVCINTRAPERIOD(_config) \
    MFX_OMX_AT__OMX_STRUCT(_config); \
    MFX_OMX_AUTO_TRACE_I32(_config.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_config.nIDRPeriod); \
    MFX_OMX_AUTO_TRACE_I32(_config.nPFrames);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_INTEL_AVC_DECODE_SETTINGS(_intel_avc_decode_settings) \
    MFX_OMX_AT__OMX_STRUCT(_intel_avc_decode_settings); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avc_decode_settings.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avc_decode_settings.nMaxNumberOfReferenceFrame); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avc_decode_settings.nMaxWidth); \
    MFX_OMX_AUTO_TRACE_I32(_intel_avc_decode_settings.nMaxHeight);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_INTEL_REQUEST_BALCK_FRAME_POINTER(_request_black_frame_pointer) \
    MFX_OMX_AT__OMX_STRUCT(_request_black_frame_pointer); \
    MFX_OMX_AUTO_TRACE_I32(_request_black_frame_pointer.nPortIndex); \
    MFX_OMX_AUTO_TRACE_P(_request_black_frame_pointer.nFramePointer);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_AVCTYPE(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.nSliceHeaderSpacing); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPFrames); \
    MFX_OMX_AUTO_TRACE_I32(_params.nBFrames); \
    MFX_OMX_AUTO_TRACE_I32(_params.bUseHadamard); \
    MFX_OMX_AUTO_TRACE_I32(_params.nRefFrames); \
    MFX_OMX_AUTO_TRACE_I32(_params.nRefIdx10ActiveMinus1); \
    MFX_OMX_AUTO_TRACE_I32(_params.nRefIdx11ActiveMinus1); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEnableUEP); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEnableFMO); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEnableASO); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEnableRS); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEnableFMO); \
    MFX_OMX_AUTO_TRACE_I32(_params.eProfile); \
    MFX_OMX_AUTO_TRACE_U32(_params.eLevel); \
    MFX_OMX_AUTO_TRACE_U32(_params.nAllowedPictureTypes); \
    MFX_OMX_AUTO_TRACE_I32(_params.bFrameMBsOnly); \
    MFX_OMX_AUTO_TRACE_I32(_params.bMBAFF); \
    MFX_OMX_AUTO_TRACE_I32(_params.bEntropyCodingCABAC); \
    MFX_OMX_AUTO_TRACE_I32(_params.bWeightedPPrediction); \
    MFX_OMX_AUTO_TRACE_I32(_params.nWeightedBipredicitonMode); \
    MFX_OMX_AUTO_TRACE_I32(_params.bconstIpred); \
    MFX_OMX_AUTO_TRACE_I32(_params.bDirect8x8Inference); \
    MFX_OMX_AUTO_TRACE_I32(_params.bDirectSpatialTemporal); \
    MFX_OMX_AUTO_TRACE_I32(_params.nCabacInitIdc); \
    MFX_OMX_AUTO_TRACE_U32(_params.eLoopFilterMode);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_HEVCTYPE(_params) \
    MFX_OMX_AT__OMX_STRUCT(_params); \
    MFX_OMX_AUTO_TRACE_I32(_params.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_params.eProfile); \
    MFX_OMX_AUTO_TRACE_U32(_params.eLevel);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_PORTFORMATTYPE(_port) \
    MFX_OMX_AT__OMX_STRUCT(_port); \
    MFX_OMX_AUTO_TRACE_I32(_port.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_port.nIndex); \
    MFX_OMX_AUTO_TRACE_U32(_port.eCompressionFormat); \
    MFX_OMX_AUTO_TRACE_U32(_port.eColorFormat); \
    MFX_OMX_AUTO_TRACE_U32(_port.xFramerate);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_PARAM_PROFILELEVELTYPE(_profile) \
    MFX_OMX_AT__OMX_STRUCT(_profile); \
    MFX_OMX_AUTO_TRACE_I32(_profile.nPortIndex); \
    MFX_OMX_AUTO_TRACE_U32(_profile.eProfile); \
    MFX_OMX_AUTO_TRACE_U32(_profile.eLevel); \
    MFX_OMX_AUTO_TRACE_I32(_profile.nProfileIndex);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_BUFFERHEADERTYPE(_buf) \
    MFX_OMX_AT__OMX_STRUCT(_buf); \
    MFX_OMX_AUTO_TRACE_P(_buf.pBuffer); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nAllocLen); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nFilledLen); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nOffset); \
    MFX_OMX_AUTO_TRACE_P(_buf.pAppPrivate); \
    MFX_OMX_AUTO_TRACE_P(_buf.pPlatformPrivate); \
    MFX_OMX_AUTO_TRACE_P(_buf.pInputPortPrivate); \
    MFX_OMX_AUTO_TRACE_P(_buf.pOutputPortPrivate); \
    MFX_OMX_AUTO_TRACE_P(_buf.hMarkTargetComponent); \
    MFX_OMX_AUTO_TRACE_P(_buf.pMarkData); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nTickCount); \
    MFX_OMX_AUTO_TRACE_I64(_buf.nTimeStamp); \
    MFX_OMX_AUTO_TRACE_U32(_buf.nFlags); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nOutputPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_buf.nInputPortIndex);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxFrameInfo(_info) \
    MFX_OMX_AUTO_TRACE_U32(_info.FourCC); \
    MFX_OMX_AUTO_TRACE_I32(_info.Width); \
    MFX_OMX_AUTO_TRACE_I32(_info.Height); \
    MFX_OMX_AUTO_TRACE_I32(_info.CropX); \
    MFX_OMX_AUTO_TRACE_I32(_info.CropY); \
    MFX_OMX_AUTO_TRACE_I32(_info.CropW); \
    MFX_OMX_AUTO_TRACE_I32(_info.CropH); \
    MFX_OMX_AUTO_TRACE_I32(_info.FrameRateExtN); \
    MFX_OMX_AUTO_TRACE_I32(_info.FrameRateExtD); \
    MFX_OMX_AUTO_TRACE_I32(_info.AspectRatioW); \
    MFX_OMX_AUTO_TRACE_I32(_info.AspectRatioH); \
    MFX_OMX_AUTO_TRACE_I32(_info.PicStruct); \
    MFX_OMX_AUTO_TRACE_I32(_info.ChromaFormat);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxInfoMFX_vpp(_info) \
    MFX_OMX_AT__mfxFrameInfo(_info.In) \
    MFX_OMX_AT__mfxFrameInfo(_info.Out)

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxInfoMFX_dec(_info) \
    MFX_OMX_AT__mfxFrameInfo(_info.FrameInfo); \
    MFX_OMX_AUTO_TRACE_U32(_info.CodecId); \
    MFX_OMX_AUTO_TRACE_U32(_info.CodecProfile); \
    MFX_OMX_AUTO_TRACE_U32(_info.CodecLevel); \
    MFX_OMX_AUTO_TRACE_I32(_info.NumThread); \
    MFX_OMX_AUTO_TRACE_I32(_info.DecodedOrder); \
    MFX_OMX_AUTO_TRACE_I32(_info.ExtendedPicStruct);

/*------------------------------------------------------------------------------*/

#if MFX_OMX_DEBUG == MFX_OMX_YES
    #define MFX_OMX_AT__mfxInfoMFX_enc_RC_method(_info) \
        MFX_OMX_AUTO_TRACE_I32(_info.RateControlMethod); \
        if ((MFX_RATECONTROL_CBR == _info.RateControlMethod) || \
            (MFX_RATECONTROL_VBR == _info.RateControlMethod)) \
        { \
            MFX_OMX_AUTO_TRACE_I32(_info.InitialDelayInKB); \
            MFX_OMX_AUTO_TRACE_I32(_info.BufferSizeInKB); \
            MFX_OMX_AUTO_TRACE_I32(_info.TargetKbps); \
            MFX_OMX_AUTO_TRACE_I32(_info.MaxKbps); \
        } \
        else if (MFX_RATECONTROL_CQP == _info.RateControlMethod) \
        { \
            MFX_OMX_AUTO_TRACE_I32(_info.QPI); \
            MFX_OMX_AUTO_TRACE_I32(_info.BufferSizeInKB); \
            MFX_OMX_AUTO_TRACE_I32(_info.QPP); \
            MFX_OMX_AUTO_TRACE_I32(_info.QPB); \
        } /* \
        else if (MFX_RATECONTROL_AVBR == _info.RateControlMethod) \
        { \
            MFX_OMX_AUTO_TRACE_I32(_info.Accuracy); \
            MFX_OMX_AUTO_TRACE_I32(_info.BufferSizeInKB); \
            MFX_OMX_AUTO_TRACE_I32(_info.TargetKbps); \
            MFX_OMX_AUTO_TRACE_I32(_info.Convergence); \
        } */ \
        else \
        { \
            MFX_OMX_AUTO_TRACE_MSG("Unknown Rate Control Method!"); \
            MFX_OMX_AUTO_TRACE_I32(_info.BufferSizeInKB); \
        }
#else // #if MFX_OMX_DEBUG == MFX_OMX_YES
    #define MFX_OMX_AT__mfxInfoMFX_enc_RC_method(_info)
#endif // #if MFX_OMX_DEBUG == MFX_OMX_YES

#define MFX_OMX_AT__mfxInfoMFX_enc(_info) \
    MFX_OMX_AUTO_TRACE_I32(_info.LowPower); \
    MFX_OMX_AUTO_TRACE_I32(_info.BRCParamMultiplier); \
    MFX_OMX_AT__mfxFrameInfo(_info.FrameInfo); \
    MFX_OMX_AUTO_TRACE_U32(_info.CodecId); \
    MFX_OMX_AUTO_TRACE_I32(_info.CodecProfile); \
    MFX_OMX_AUTO_TRACE_I32(_info.CodecLevel); \
    MFX_OMX_AUTO_TRACE_I32(_info.NumThread); \
    MFX_OMX_AUTO_TRACE_I32(_info.TargetUsage); \
    MFX_OMX_AUTO_TRACE_I32(_info.GopPicSize); \
    MFX_OMX_AUTO_TRACE_I32(_info.GopRefDist); \
    MFX_OMX_AUTO_TRACE_I32(_info.GopOptFlag); \
    MFX_OMX_AUTO_TRACE_I32(_info.IdrInterval); \
    MFX_OMX_AT__mfxInfoMFX_enc_RC_method(_info); \
    MFX_OMX_AUTO_TRACE_I32(_info.NumSlice); \
    MFX_OMX_AUTO_TRACE_I32(_info.NumRefFrame); \
    MFX_OMX_AUTO_TRACE_I32(_info.EncodedOrder);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxVideoParam_dec(_params) \
    MFX_OMX_AUTO_TRACE_I32(_params.AsyncDepth); \
    MFX_OMX_AT__mfxInfoMFX_dec(_params.mfx); \
    MFX_OMX_AUTO_TRACE_I32(_params.Protected); \
    MFX_OMX_AUTO_TRACE_U32(_params.IOPattern); \
    MFX_OMX_AUTO_TRACE_P(_params.ExtParam); \
    MFX_OMX_AUTO_TRACE_I32(_params.NumExtParam);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtCodingOption(_opt) \
    MFX_OMX_AUTO_TRACE_I32(_opt.reserved1); \
    MFX_OMX_AUTO_TRACE_I32(_opt.RateDistortionOpt); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MECostType); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MESearchType); \
    /*mfxI16Pair  MVSearchWindow;*/ \
    MFX_OMX_AUTO_TRACE_I32(_opt.EndOfSequence); \
    MFX_OMX_AUTO_TRACE_I32(_opt.FramePicture); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.CAVLC); \
    MFX_OMX_AUTO_TRACE_I32(_opt.reserved2[0]); \
    MFX_OMX_AUTO_TRACE_I32(_opt.reserved2[1]); \
    MFX_OMX_AUTO_TRACE_I32(_opt.RecoveryPointSEI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.ViewOutput); \
    MFX_OMX_AUTO_TRACE_I32(_opt.NalHrdConformance); \
    MFX_OMX_AUTO_TRACE_I32(_opt.SingleSeiNalUnit); \
    MFX_OMX_AUTO_TRACE_I32(_opt.VuiVclHrdParameters); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.RefPicListReordering); \
    MFX_OMX_AUTO_TRACE_I32(_opt.ResetRefList); \
    MFX_OMX_AUTO_TRACE_I32(_opt.RefPicMarkRep); \
    MFX_OMX_AUTO_TRACE_I32(_opt.FieldOutput); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.IntraPredBlockSize); \
    MFX_OMX_AUTO_TRACE_I32(_opt.InterPredBlockSize); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MVPrecision); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxDecFrameBuffering); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.AUDelimiter); \
    MFX_OMX_AUTO_TRACE_I32(_opt.EndOfStream); \
    MFX_OMX_AUTO_TRACE_I32(_opt.PicTimingSEI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.VuiNalHrdParameters);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtCodingOption2(_opt) \
    MFX_OMX_AUTO_TRACE_I32(_opt.IntRefType); \
    MFX_OMX_AUTO_TRACE_I32(_opt.IntRefCycleSize); \
    MFX_OMX_AUTO_TRACE_I32(_opt.IntRefQPDelta); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxFrameSize); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxSliceSize); \
    \
    MFX_OMX_AUTO_TRACE_I32(_opt.BitrateLimit); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MBBRC); \
    MFX_OMX_AUTO_TRACE_I32(_opt.ExtBRC); \
    MFX_OMX_AUTO_TRACE_I32(_opt.LookAheadDepth); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Trellis); \
    MFX_OMX_AUTO_TRACE_I32(_opt.RepeatPPS); \
    MFX_OMX_AUTO_TRACE_I32(_opt.BRefType); \
    MFX_OMX_AUTO_TRACE_I32(_opt.AdaptiveI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.AdaptiveB); \
    MFX_OMX_AUTO_TRACE_I32(_opt.LookAheadDS); \
    MFX_OMX_AUTO_TRACE_I32(_opt.NumMbPerSlice); \
    MFX_OMX_AUTO_TRACE_I32(_opt.SkipFrame); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MinQPI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxQPI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MinQPP); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxQPP); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MinQPB); \
    MFX_OMX_AUTO_TRACE_I32(_opt.MaxQPB); \
    MFX_OMX_AUTO_TRACE_I32(_opt.FixedFrameRate); \
    MFX_OMX_AUTO_TRACE_I32(_opt.DisableDeblockingIdc); \
    MFX_OMX_AUTO_TRACE_I32(_opt.DisableVUI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.BufferingPeriodSEI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.EnableMAD); \
    MFX_OMX_AUTO_TRACE_I32(_opt.UseRawRef);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtCodingOption3(_opt) \
    MFX_OMX_AUTO_TRACE_I32(_opt.NumSliceI); \
    MFX_OMX_AUTO_TRACE_I32(_opt.NumSliceP); \
    MFX_OMX_AUTO_TRACE_I32(_opt.NumSliceB); \
    MFX_OMX_AUTO_TRACE_I32(_opt.WinBRCMaxAvgKbps); \
    MFX_OMX_AUTO_TRACE_I32(_opt.WinBRCSize); \
    MFX_OMX_AUTO_TRACE_I32(_opt.QVBRQuality);
    /*MFX_OMX_AUTO_TRACE_I32(_opt.reserved[249];*/

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtEncoderResetOption(_opt) \
    MFX_OMX_AUTO_TRACE_I32(_opt.StartNewSequence); \
    /*MFX_OMX_AUTO_TRACE_I32(_opt.reserved[11];*/

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtEncoderTemporalLayersOption(_opt) \
    MFX_OMX_AUTO_TRACE_I32(_opt.BaseLayerPID); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[0].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[1].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[2].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[3].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[4].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[5].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[6].Scale); \
    MFX_OMX_AUTO_TRACE_I32(_opt.Layer[7].Scale);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxExtParams(_num, _params) \
    if(_params) \
    { \
        for (int i = 0; i < _num; ++i) \
        { \
            MFX_OMX_AUTO_TRACE_U32(_params[i]->BufferId); \
            MFX_OMX_AUTO_TRACE_I32(_params[i]->BufferSz); \
            switch (_params[i]->BufferId) \
            { \
              case MFX_EXTBUFF_CODING_OPTION: \
                MFX_OMX_AT__mfxExtCodingOption((*((mfxExtCodingOption*)_params[i])));\
                break; \
              case MFX_EXTBUFF_CODING_OPTION2: \
                MFX_OMX_AT__mfxExtCodingOption2((*((mfxExtCodingOption2*)_params[i])));\
                break; \
              case MFX_EXTBUFF_CODING_OPTION3: \
                MFX_OMX_AT__mfxExtCodingOption3((*((mfxExtCodingOption3*)_params[i])));\
                break; \
              case MFX_EXTBUFF_ENCODER_RESET_OPTION: \
                MFX_OMX_AT__mfxExtEncoderResetOption((*((mfxExtEncoderResetOption*)_params[i])));\
                break; \
              case MFX_EXTBUFF_AVC_TEMPORAL_LAYERS: \
                MFX_OMX_AT__mfxExtEncoderTemporalLayersOption((*((mfxExtAvcTemporalLayers*)_params[i]))); \
                break; \
              default: \
                MFX_OMX_AUTO_TRACE_MSG("unknown ext buffer"); \
                break; \
            }; \
        } \
    }

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxVideoParam_enc(_params) \
    MFX_OMX_AUTO_TRACE_I32(_params.AsyncDepth); \
    MFX_OMX_AT__mfxInfoMFX_enc(_params.mfx); \
    MFX_OMX_AUTO_TRACE_I32(_params.Protected); \
    MFX_OMX_AUTO_TRACE_U32(_params.IOPattern); \
    MFX_OMX_AUTO_TRACE_I32(_params.NumExtParam); \
    MFX_OMX_AUTO_TRACE_P(_params.ExtParam); \
    MFX_OMX_AT__mfxExtParams(_params.NumExtParam, _params.ExtParam)

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__mfxBitstream(_bst) \
    MFX_OMX_AUTO_TRACE_I64(_bst.TimeStamp); \
    MFX_OMX_AUTO_TRACE_P(_bst.Data); \
    MFX_OMX_AUTO_TRACE_I32(_bst.DataOffset); \
    MFX_OMX_AUTO_TRACE_I32(_bst.DataLength); \
    MFX_OMX_AUTO_TRACE_I32(_bst.MaxLength); \
    MFX_OMX_AUTO_TRACE_I32(_bst.PicStruct); \
    MFX_OMX_AUTO_TRACE_I32(_bst.FrameType); \
    MFX_OMX_AUTO_TRACE_I32(_bst.DataFlag);

/*------------------------------------------------------------------------------*/

#define MFX_OMX_AT__OMX_VIDEO_TEMPORAL_LAYER_COUNT(_config) \
    MFX_OMX_AT__OMX_STRUCT(_config); \
    MFX_OMX_AUTO_TRACE_I32(_config.nPortIndex); \
    MFX_OMX_AUTO_TRACE_I32(_config.nTemproalLayerCount);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_DEFS_H__

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

#include "mfx_omx_defs.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_debug"

/*------------------------------------------------------------------------------*/

#if MFX_OMX_DEBUG == MFX_OMX_YES

/*------------------------------------------------------------------------------*/

static const char* g_debug_pattern[] =
{
    "omx2mfx"
    ,"omx2mfx_config"
    ,"mfx2omx_config"
    ,"SetParameter"
    ,"GetParameter"
    ,"SetConfig"
    ,"GetConfig"
    ,"ValidateConfig"
    ,"InitEncoder"
    ,"ReinitCodec"
    ,"EnableVp"
    //,"GetGrallocSurface"
    //"ProcessFrame"
    //,"ProcessFrameEnc"
    //,"SendBitstream"
    //,"GetOMXBuffer"
    //,"MainThread"
    //,"GetGrallocHandle"
    //,"CloseCodec"
};

/*------------------------------------------------------------------------------*/

static bool is_matched(const char* str)
{
    if (0 == MFX_OMX_GET_ARRAY_SIZE(g_debug_pattern)) return true; // match all
    if (!str) return false;

    for (int i=0; i < MFX_OMX_GET_ARRAY_SIZE(g_debug_pattern); ++i)
    {
        if (strstr(str, g_debug_pattern[i])) return true;
    }
    return false;
}

/*------------------------------------------------------------------------------*/

mfx_omx_trace::mfx_omx_trace(const char* _modulename, const char* _function, const char* _taskname)
{
    modulename = _modulename;
    function = _function;
    taskname = _taskname;

    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: +\n", modulename, function, taskname);
        else
            fprintf(g_dbg_file, "%s: %s: +\n", modulename, function);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: +", modulename, function, taskname);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: +", modulename, function);
#endif
}

/*------------------------------------------------------------------------------*/

mfx_omx_trace::~mfx_omx_trace(void)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: -\n", modulename, function, taskname);
        else
            fprintf(g_dbg_file, "%s: %s: -\n", modulename, function);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: -", modulename, function, taskname);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: -", modulename, function);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_i32(const char* name, mfxI32 value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = %d\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = %d\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = %d", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = %d", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_u32(const char* name, mfxU32 value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = 0x%x\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = 0x%x\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = 0x%x", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = 0x%x", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_i64(const char* name, mfxI64 value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = %ld\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = %ld\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = %lld", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = %lld", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_f64(const char* name, mfxF64 value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = %f\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = %f\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = %f", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = %f", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_p(const char* name, void* value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = %p\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = %p\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = %p", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = %p", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_s(const char* name, const char* value)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s = '%s'\n", modulename, function, taskname, name, value);
        else
            fprintf(g_dbg_file, "%s: %s: %s = '%s'\n", modulename, function, name, value);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s = %s", modulename, function, taskname, name, value);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s = %s", modulename, function, name, value);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_trace::printf_msg(const char* msg)
{
    if (!is_matched(function)) return;

#if defined(MFX_OMX_STDOUT)
    if (g_dbg_file)
    {
        if (taskname)
            fprintf(g_dbg_file, "%s: %s: %s: %s\n", modulename, function, taskname, msg);
        else
            fprintf(g_dbg_file, "%s: %s: %s\n", modulename, function, msg);
        fflush(g_dbg_file);
    }
#else
    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: %s", modulename, function, taskname, msg);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s", modulename, function, msg);
#endif
}

/*------------------------------------------------------------------------------*/

#endif // #if MFX_OMX_DEBUG == MFX_OMX_YES

/*------------------------------------------------------------------------------*/

#if MFX_OMX_PERF == MFX_OMX_YES

/*------------------------------------------------------------------------------*/

#define MAX_CPU_FREQUENCY 1460000.0  // in kHz

static mfxU64 rdtsc(void)
{
    mfxU32 a, d;

    __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

    return ((mfxU64)a) | (((mfxU64)d) << 32);
}

/*------------------------------------------------------------------------------*/

mfx_omx_perf::mfx_omx_perf(const char* _modulename, const char* _function, const char* _taskname)
{
    modulename = _modulename;
    function = _function;
    taskname = _taskname;

    start = rdtsc();
}

/*------------------------------------------------------------------------------*/

mfx_omx_perf::~mfx_omx_perf(void)
{
    float task_time = (rdtsc() - start) / MAX_CPU_FREQUENCY; // in ms

    if (taskname)
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: %s: time = %f ms", modulename, function, taskname, task_time);
    else
        __android_log_print(MFX_OMX_LOG_LEVEL, MFX_OMX_LOG_TAG, "%s: %s: time = %f ms", modulename, function, task_time);
}

/*------------------------------------------------------------------------------*/

#endif // #if MFX_OMX_PERF == MFX_OMX_YES

struct CodeStringTable
{
    mfxStatus status;
    const char* string;
};

static CodeStringTable g_StatusNames[] =
{
    { MFX_ERR_NONE,                     "MFX_ERR_NONE"},
    { MFX_ERR_UNKNOWN,                  "MFX_ERR_UNKNOWN"},
    { MFX_ERR_NULL_PTR,                 "MFX_ERR_NULL_PTR"},
    { MFX_ERR_UNSUPPORTED,              "MFX_ERR_UNSUPPORTED"},
    { MFX_ERR_MEMORY_ALLOC,             "MFX_ERR_MEMORY_ALLOC"},
    { MFX_ERR_NOT_ENOUGH_BUFFER,        "MFX_ERR_NOT_ENOUGH_BUFFER"},
    { MFX_ERR_INVALID_HANDLE,           "MFX_ERR_INVALID_HANDLE"},
    { MFX_ERR_LOCK_MEMORY,              "MFX_ERR_LOCK_MEMORY"},
    { MFX_ERR_NOT_INITIALIZED,          "MFX_ERR_NOT_INITIALIZED"},
    { MFX_ERR_NOT_FOUND,                "MFX_ERR_NOT_FOUND"},
    { MFX_ERR_MORE_DATA,                "MFX_ERR_MORE_DATA"},
    { MFX_ERR_MORE_SURFACE,             "MFX_ERR_MORE_SURFACE"},
    { MFX_ERR_ABORTED,                  "MFX_ERR_ABORTED"},
    { MFX_ERR_DEVICE_LOST,              "MFX_ERR_DEVICE_LOST"},
    { MFX_ERR_INCOMPATIBLE_VIDEO_PARAM, "MFX_ERR_INCOMPATIBLE_VIDEO_PARAM"},
    { MFX_ERR_INVALID_VIDEO_PARAM,      "MFX_ERR_INVALID_VIDEO_PARAM"},
    { MFX_ERR_UNDEFINED_BEHAVIOR,       "MFX_ERR_UNDEFINED_BEHAVIOR"},
    { MFX_ERR_DEVICE_FAILED,            "MFX_ERR_DEVICE_FAILED"},
    { MFX_ERR_MORE_BITSTREAM,           "MFX_ERR_MORE_BITSTREAM"},
    { MFX_ERR_INCOMPATIBLE_AUDIO_PARAM, "MFX_ERR_INCOMPATIBLE_AUDIO_PARAM"},
    { MFX_ERR_INVALID_AUDIO_PARAM,      "MFX_ERR_INVALID_AUDIO_PARAM"},
    { MFX_ERR_GPU_HANG,                 "MFX_ERR_GPU_HANG"},
    { MFX_ERR_REALLOC_SURFACE,          "MFX_ERR_REALLOC_SURFACE"},
    { MFX_WRN_IN_EXECUTION,             "MFX_WRN_IN_EXECUTION"},
    { MFX_WRN_DEVICE_BUSY,              "MFX_WRN_DEVICE_BUSY"},
    { MFX_WRN_VIDEO_PARAM_CHANGED,      "MFX_WRN_VIDEO_PARAM_CHANGED"},
    { MFX_WRN_PARTIAL_ACCELERATION,     "MFX_WRN_PARTIAL_ACCELERATION"},
    { MFX_WRN_INCOMPATIBLE_VIDEO_PARAM, "MFX_WRN_INCOMPATIBLE_VIDEO_PARAM"},
    { MFX_WRN_VALUE_NOT_CHANGED,        "MFX_WRN_VALUE_NOT_CHANGED"},
    { MFX_WRN_OUT_OF_RANGE,             "MFX_WRN_OUT_OF_RANGE"},
    { MFX_WRN_FILTER_SKIPPED,           "MFX_WRN_FILTER_SKIPPED"},
    { MFX_WRN_INCOMPATIBLE_AUDIO_PARAM, "MFX_WRN_INCOMPATIBLE_AUDIO_PARAM"},
    { MFX_TASK_WORKING,                 "MFX_TASK_WORKING"},
    { MFX_TASK_BUSY,                    "MFX_TASK_BUSY"}
};

const char* mfx_omx_code_to_string(mfxStatus sts)
{
    mfxU32 table_size = MFX_OMX_GET_ARRAY_SIZE(g_StatusNames);
    for (mfxU32 i = 0; i < table_size; ++i)
    {
        if (g_StatusNames[i].status == sts) return g_StatusNames[i].string;
    }
    return "Unknown status code";
}

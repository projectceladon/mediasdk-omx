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

#ifndef __MFX_OMX_UTILS_H__
#define __MFX_OMX_UTILS_H__

#include <vector>
#include "mfx_omx_types.h"
#include "mfx_omx_vm.h"

#define RCNEGATE(x)  ( -(x) )
#define RSIZE_MAX_MEM      ( 256UL << 20 )     /* 256MB */
#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */

typedef enum {
    ESNULLP  = 400,
    ESZEROL  = 401,
    ESLEMIN  = 402,
    ESLEMAX  = 403,
    ESOVRLP  = 404,
    ESEMPTY  = 405,
    ESNOSPC  = 406,
    ESUNTERM = 407,
    ESNODIFF = 408,
    ESNOTFND = 409,
    EOK = 0
} safec_errors;

/*------------------------------------------------------------------------------*/

// The functions below are necessary to avoid warnings :
// "comparison of two values with different enumeration types [-Wenum-compare]".
// Framework have enums with its own values and reserves values for vendors.
// (for example: /frameworks/native/headers/media_plugin/media/openmax/OMX_Index.h - OMX_INDEXTYPE)
// On MSDK side some values added in its own enum with the other names.

inline bool operator==(OMX_INTELINDEXEXTTYPE left, OMX_INDEXTYPE right)
{
    return (static_cast<OMX_INDEXTYPE>(left) == right);
}

inline bool operator==(OMX_INDEXEXTTYPE left, OMX_INDEXTYPE right)
{
    return (static_cast<OMX_INDEXTYPE>(left) == right);
}

inline bool operator==(OMX_VIDEO_INTEL_CONTROL_RATE left, OMX_VIDEO_CONTROLRATETYPE right)
{
    return (static_cast<OMX_VIDEO_CONTROLRATETYPE>(left) == right);
}

inline bool operator==(OMX_VIDEO_CONTROLRATETYPE left, OMX_VIDEO_INTEL_CONTROL_RATE right)
{
    return (left == static_cast<OMX_VIDEO_CONTROLRATETYPE>(right));
}

/*------------------------------------------------------------------------------*/

// You can find OMX Error codes in OMX_Core.h
// folder: /frameworks/native/include/media/openmax/
inline OMX_ERRORTYPE ErrorStatusMfxToOmx(mfxStatus mfxSts)
{
    switch (mfxSts)
    {
        case MFX_ERR_NONE:
        {
            return OMX_ErrorNone;           // 0
        }
        case MFX_ERR_UNKNOWN:
        case MFX_ERR_UNDEFINED_BEHAVIOR:
        {
            return OMX_ErrorUndefined;      // 0x80001001
        }
        case MFX_ERR_INVALID_HANDLE:
        case MFX_ERR_NULL_PTR:
        case MFX_ERR_INCOMPATIBLE_VIDEO_PARAM:
        case MFX_ERR_INVALID_VIDEO_PARAM:
        {
            return OMX_ErrorBadParameter;   // 0x80001005
        }
        case MFX_ERR_UNSUPPORTED:
        {
            return OMX_ErrorNotImplemented; // 0x80001006
        }
        case MFX_ERR_DEVICE_LOST:
        case MFX_ERR_DEVICE_FAILED:
        case MFX_ERR_GPU_HANG:
        {
            return OMX_ErrorHardware;       // 0x80001009
        }
        default:
        {
            return OMX_ErrorUndefined;
        }
    }
}

/*------------------------------------------------------------------------------*/

inline mfxStatus va_to_mfx_status(VAStatus vaSts)
{
    switch (vaSts)
    {
        case VA_STATUS_SUCCESS:
        {
            return MFX_ERR_NONE;
        }
        case VA_STATUS_ERROR_OPERATION_FAILED:
        {
            return MFX_ERR_ABORTED;
        }
        case VA_STATUS_ERROR_DECODING_ERROR:
        case VA_STATUS_ERROR_ENCODING_ERROR:
        {
            return MFX_ERR_DEVICE_FAILED;
        }
        case VA_STATUS_ERROR_ALLOCATION_FAILED:
        {
            return MFX_ERR_MEMORY_ALLOC;
        }
        case VA_STATUS_ERROR_INVALID_DISPLAY:
        case VA_STATUS_ERROR_INVALID_CONFIG:
        case VA_STATUS_ERROR_INVALID_CONTEXT:
        case VA_STATUS_ERROR_INVALID_SURFACE:
        case VA_STATUS_ERROR_INVALID_BUFFER:
        case VA_STATUS_ERROR_INVALID_IMAGE:
        case VA_STATUS_ERROR_INVALID_SUBPICTURE:
        case VA_STATUS_ERROR_INVALID_PARAMETER:
        case VA_STATUS_ERROR_INVALID_IMAGE_FORMAT:
        {
            return MFX_ERR_INVALID_HANDLE;
        }
        case VA_STATUS_ERROR_ATTR_NOT_SUPPORTED:
        case VA_STATUS_ERROR_UNSUPPORTED_PROFILE:
        case VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT:
        case VA_STATUS_ERROR_UNSUPPORTED_RT_FORMAT:
        case VA_STATUS_ERROR_UNSUPPORTED_BUFFERTYPE:
        case VA_STATUS_ERROR_FLAG_NOT_SUPPORTED:
        case VA_STATUS_ERROR_RESOLUTION_NOT_SUPPORTED:
        case VA_STATUS_ERROR_UNIMPLEMENTED:
        {
            return MFX_ERR_UNSUPPORTED;
        }
        default:
        {
            return MFX_ERR_UNKNOWN;
        }
    }
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern mfxVersion g_MfxVersion;
extern mfxSyncPoint* g_NilSyncPoint;
extern mfxU32 g_OmxLogLevel;

/*------------------------------------------------------------------------------*/

extern size_t mfx_omx_dump(
    const void *ptr, size_t size, size_t nmemb,
    FILE *stream);

extern void mfx_omx_dump_YUV_from_NV12_data(
    FILE* f, mfxFrameData * pData, mfxFrameInfo* info, mfxU32 p);

extern void mfx_omx_dump_YUV_from_P010_data(
    FILE* f, mfxFrameData * pData, mfxFrameInfo* pInfo);

extern void mfx_omx_dump_RGB_from_RGB4_data(
    FILE* f, mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 pitch);

extern void mfx_omx_copy_nv12(
    mfxFrameSurface1* dst, mfxFrameSurface1* src);

extern mfxF64 mfx_omx_get_framerate(mfxU32 fr_n, mfxU32 fr_d);

extern bool mfx_omx_are_framerates_equal(
    mfxU32 fr1_n, mfxU32 fr1_d,
    mfxU32 fr2_n, mfxU32 fr2_d);

extern void mfx_omx_reset_bitstream(mfxBitstream* pBitstream);

extern mfxStatus mfx_omx_get_metadatabuffer_info(mfxU8* data, mfxU32 size, MetadataBuffer* pInfo);

extern int strcmp_s(const char *dest, size_t dmax, const char *src, int *indicator);

extern int strcpy_s(char *dest, size_t dmax, const char *src);

extern size_t strnlen_s(const char *dest, size_t dmax);

int snprintf_s_ss(char *dest, size_t dmax, const char *format, const char *s1, const char *s2);

int snprintf_s_p(char *dest, size_t dmax, const char *format, void *ptr);

/*------------------------------------------------------------------------------*/

buffer_handle_t GetGrallocHandle(OMX_BUFFERHEADERTYPE* pBuffer);
MfxMetadataBufferType GetMetadataType(OMX_BUFFERHEADERTYPE* pBuffer);

/*------------------------------------------------------------------------------*/

mfxU32 GetBufferSize(mfxVideoParam const & par);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

template <typename T>
bool IsStructVersionValid(const T* s, OMX_U32 nSize, OMX_U32 nVersion)
{
#if MFX_OMX_ROUGH_VERSION_CHECK == MFX_OMX_YES
    return (s->nSize == nSize) && (s->nVersion.nVersion == nVersion);
#else
    OMX_VERSIONTYPE version;

    version.nVersion = nVersion;
    return (s->nSize == nSize) &&
           (s->nVersion.s.nVersionMajor == version.s.nVersionMajor) &&
           (s->nVersion.s.nVersionMinor == version.s.nVersionMinor);
#endif
}

/*------------------------------------------------------------------------------*/

template <typename T>
bool IsStructVersionValid(const T* s, OMX_U32 nSize, OMX_VERSIONTYPE nVersion)
{
#if MFX_OMX_ROUGH_VERSION_CHECK == MFX_OMX_YES
    return (s->nSize == nSize) && (s->nVersion.nVersion == nVersion.nVersion);
#else
    return (s->nSize == nSize) &&
           (s->nVersion.s.nVersionMajor == nVersion.s.nVersionMajor) &&
           (s->nVersion.s.nVersionMinor == nVersion.s.nVersionMinor);
#endif
}

/*------------------------------------------------------------------------------*/

template <typename T>
void SetStructVersion(T* s)
{
    s->nSize = sizeof(T);
    s->nVersion.nVersion = OMX_VERSION;
}

/*------------------------------------------------------------------------------*/

template <typename T>
class MfxOmxRing
{
public:
    MfxOmxRing(void);
    ~MfxOmxRing(void);
    T* Add(T* item); // returns pointer to newly added item
    T* Get(T* item); // returns pointer to returned item, i.e. function parameter
    T* Get(T* item, T& nil_item); // the same as Get, but returns nil_item on failure
    inline mfxU32 GetItemsCount(void);
protected:
    T* m_ring;
    T* m_read_item;
    T* m_write_item;
    mfxU32 m_ring_size;
    mfxU32 m_items_count;
    MfxOmxMutex m_mutex;
private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxRing)
};

/*------------------------------------------------------------------------------*/

template <typename T>
MfxOmxRing<T>::MfxOmxRing(void)
{
    m_ring_size = 0;
    m_ring = NULL;
    m_read_item = NULL;
    m_write_item = NULL;
    m_items_count = 0;
}

/*------------------------------------------------------------------------------*/

template <typename T>
MfxOmxRing<T>::~MfxOmxRing(void)
{
    if (m_ring) free(m_ring);
}

/*------------------------------------------------------------------------------*/

template <typename T>
T* MfxOmxRing<T>::Add(T* item)
{
    MfxOmxAutoLock lock(m_mutex);
    T* added_item = NULL;

    if (!item) return NULL;
    if (m_write_item) // there is already free space in the ring
    {
        // adding new item
        *m_write_item = *item;
        added_item = m_write_item;
    }
    else // no free space, need to realloc
    {
        T *ring = (T*)realloc(m_ring, (m_ring_size+1)*sizeof(T));
        if (ring)
        {
            // resetting ring pointers
            if (m_read_item) m_read_item = ring + (m_read_item - m_ring);
            m_ring = ring;
            // shifting old part of the ring
            m_write_item = &(m_ring[m_ring_size]);
            if (m_read_item)
            {
                for (; m_read_item && (m_write_item != m_read_item); --m_write_item)
                {
                    *m_write_item = m_write_item[-1];
                }
                ++m_read_item;
            }
            ++m_ring_size;
            // adding new item
            *m_write_item = *item;
            added_item = m_write_item;
        }
    }
    if (added_item)
    {
        ++m_items_count;
        ++m_write_item;
        if ((size_t)(m_write_item - m_ring) >= (size_t)m_ring_size) m_write_item = m_ring;
        if (!m_read_item) m_read_item = added_item;
        if (m_write_item == m_read_item) m_write_item = NULL;
    }
    return added_item;
}

/*------------------------------------------------------------------------------*/

template <typename T>
T* MfxOmxRing<T>::Get(T* item)
{
    MfxOmxAutoLock lock(m_mutex);
    T* freed_item = NULL;

    if (!item || !m_ring || !m_read_item) return NULL;
    freed_item = m_read_item;
    *item = *m_read_item;
    --m_items_count;
    ++m_read_item;
    if ((size_t)(m_read_item - m_ring) >= (size_t)m_ring_size) m_read_item = m_ring;
    if (!m_write_item) m_write_item = freed_item;
    if (m_write_item == m_read_item) m_read_item = NULL;
    return item;
}

/*------------------------------------------------------------------------------*/

template <typename T>
T* MfxOmxRing<T>::Get(T* item, T& nil_item)
{
    T* ret_item = Get(item);

    if (!ret_item && item) *item = nil_item;
    return ret_item;
}

/*------------------------------------------------------------------------------*/

template <typename T>
mfxU32 MfxOmxRing<T>::GetItemsCount(void)
{
    return m_items_count;
}

/*------------------------------------------------------------------------------*/

template<class T> inline T * Begin(std::vector<T> & t) { return &*t.begin(); }

template<class T> inline T * End(std::vector<T> & t) { return &*t.begin() + t.size(); }

template<class T> inline void Zero(std::vector<T> & vec)   { memset(&vec[0], 0, sizeof(T) * vec.size()); }

/*------------------------------------------------------------------------------*/

class MfxOmxInputConfigAggregator: public MfxOmxInputConfig
{
public:
    MfxOmxInputConfigAggregator()
    {
        memset(static_cast<MfxOmxInputConfig*>(this), 0, sizeof(MfxOmxInputConfig));
    }
    ~MfxOmxInputConfigAggregator()
    {
        MFX_OMX_DELETE(control);
        MFX_OMX_DELETE(mfxparams);
    }
    MfxOmxInputConfigAggregator& operator+=(MfxOmxInputConfig& new_config)
    {
        if (new_config.control)
        {
            if (control)
            {
                mfxEncodeCtrl & newCtrl = *new_config.control;

                mfxU16 skipFrame = newCtrl.SkipFrame ? newCtrl.SkipFrame : control->SkipFrame;
                mfxU16 qp = newCtrl.QP ? newCtrl.QP : control->QP;
                mfxU16 frameType = newCtrl.FrameType ? newCtrl.FrameType : control->FrameType;

                *control = *new_config.control;

                control->SkipFrame = skipFrame;
                control->QP = qp;
                control->FrameType = frameType;

                MFX_OMX_DELETE(new_config.control);
            }
            else
            {
                control = new_config.control;
            }
        }
        if (new_config.mfxparams)
        {
            MFX_OMX_DELETE(mfxparams);
            mfxparams = new_config.mfxparams;
        }
        return *this;
    }
    MfxOmxInputConfig release()
    {
        MfxOmxInputConfig config;

        config.control = control;
        config.mfxparams = mfxparams;
        control = NULL;
        mfxparams = NULL;
        config.bCodecInitialized = false;

        return config;
    }

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxInputConfigAggregator)
};

struct NalUnit
{
    NalUnit() : begin(0), end(0), numZero(0)
    {}

    NalUnit(mfxU8 * b, mfxU8 * e, mfxU8 z) : begin(b), end(e), numZero(z)
    {}

    mfxU8 * begin;
    mfxU8 * end;
    mfxU32 numZero;
};

extern NalUnit GetNalUnit(mfxU8 * begin, mfxU8 * end);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern bool operator ==(NalUnit const & left, NalUnit const & right);
extern bool operator !=(NalUnit const & left, NalUnit const & right);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // #ifndef __MFX_OMX_UTILS_H__

// Copyright (c) 2017-2018 Intel Corporation
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

#include "mfx_omx_gralloc_adapter.h"

MfxOmxGrallocAdapter::MfxOmxGrallocAdapter()
    : m_pModule(NULL)
#ifdef MFX_OMX_USE_GRALLOC_1
    , m_pGralloc1Dev(NULL)
    , m_grallocGetFormat(NULL)
    , m_grallocGetNumFlexPlanes(NULL)
    , m_grallocGetByteStride(NULL)
#ifdef MFX_OMX_USE_PRIME
    , m_grallocGetPrime(NULL)
#endif
    , m_grallocGetDimensions(NULL)
    , m_grallocAllocate(NULL)
    , m_grallocRelease(NULL)
    , m_grallocLock(NULL)
    , m_grallocUnlock(NULL)
#else
    , m_pAllocDev(NULL)
    , m_pGrallocModule(NULL)
#endif
{
}

mfxStatus MfxOmxGrallocAdapter::Create(MfxOmxGrallocAdapter **ppGralloc)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (ppGralloc)
    {
        MFX_OMX_NEW(*ppGralloc, MfxOmxGrallocAdapter());
        if (*ppGralloc)
        {
            mfx_res = (*ppGralloc)->Init();
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_DELETE(*ppGralloc);
            }
        }
        else
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
    }
    else
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAdapter::Init()
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfxI32 err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &m_pModule);

    if (0 == err)
    {
#ifdef MFX_OMX_USE_GRALLOC_1
        err = gralloc1_open(m_pModule, &m_pGralloc1Dev);

        if (m_pGralloc1Dev && 0 == err)
        {
            m_grallocGetFormat =
                (GRALLOC1_PFN_GET_FORMAT)         m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_GET_FORMAT);
            m_grallocGetNumFlexPlanes =
                (GRALLOC1_PFN_GET_NUM_FLEX_PLANES)m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_GET_NUM_FLEX_PLANES);
            m_grallocGetByteStride =
                (GRALLOC1_PFN_GET_BYTE_STRIDE)    m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_GET_BYTE_STRIDE);
#ifdef MFX_OMX_USE_PRIME
            m_grallocGetPrime =
                (GRALLOC1_PFN_GET_PRIME)          m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_GET_PRIME);
#endif
            m_grallocGetDimensions =
                (GRALLOC1_PFN_GET_DIMENSIONS)     m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_GET_DIMENSIONS);
            m_grallocAllocate =
                (GRALLOC1_PFN_ALLOCATE)           m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_ALLOCATE);
            m_grallocRelease =
                (GRALLOC1_PFN_RELEASE)            m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_RELEASE);
            m_grallocLock =
                (GRALLOC1_PFN_LOCK)               m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_LOCK);
            m_grallocUnlock =
                (GRALLOC1_PFN_UNLOCK)             m_pGralloc1Dev->getFunction(m_pGralloc1Dev, GRALLOC1_FUNCTION_UNLOCK);

            if (!m_grallocGetFormat        ||
                !m_grallocGetNumFlexPlanes ||
                !m_grallocGetByteStride    ||
#ifdef MFX_OMX_USE_PRIME
                !m_grallocGetPrime         ||
#endif
                !m_grallocGetDimensions    ||
                !m_grallocAllocate         ||
                !m_grallocRelease          ||
                !m_grallocLock             ||
                !m_grallocUnlock)
            {
                gralloc1_close(m_pGralloc1Dev);
                m_pGralloc1Dev = NULL;
                mfx_res = MFX_ERR_UNKNOWN;
            }
        }
        else
        {
            mfx_res = MFX_ERR_UNKNOWN;
        }
#else
        m_pGrallocModule = (gralloc_module_t *)m_pModule;
        gralloc_open(m_pModule, &m_pAllocDev);
#endif
    }
    else
    {
        mfx_res = MFX_ERR_UNKNOWN;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

MfxOmxGrallocAdapter::~MfxOmxGrallocAdapter()
{
    MFX_OMX_AUTO_TRACE_FUNC();

#ifdef MFX_OMX_USE_GRALLOC_1
    gralloc1_close(m_pGralloc1Dev);
#else
    gralloc_close(m_pAllocDev);
#endif
}

mfxStatus MfxOmxGrallocAdapter::GetInfo(buffer_handle_t handle, intel_ufo_buffer_details_t *info)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!info)
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
#ifdef MFX_OMX_USE_GRALLOC_1
        int32_t errGetFormat        = m_grallocGetFormat(m_pGralloc1Dev, handle, &(info->format));
        int32_t errGetNumFlexPlanes = m_grallocGetNumFlexPlanes(m_pGralloc1Dev, handle, &(info->numOfPlanes));
        int32_t errGetDimensions    = m_grallocGetDimensions(m_pGralloc1Dev, handle, &(info->width), &(info->height));
        int32_t errGetPrime         = GRALLOC1_ERROR_NONE;

#ifdef MFX_OMX_USE_PRIME
        errGetPrime = m_grallocGetPrime(m_pGralloc1Dev, handle, &(info->prime));
#endif

        if (GRALLOC1_ERROR_NONE == errGetFormat        &&
            GRALLOC1_ERROR_NONE == errGetNumFlexPlanes &&
            GRALLOC1_ERROR_NONE == errGetDimensions    &&
            GRALLOC1_ERROR_NONE == errGetPrime)
        {
            int32_t errGetByteStride = m_grallocGetByteStride(m_pGralloc1Dev, handle, info->pitchArray, info->numOfPlanes);
            if (GRALLOC1_ERROR_NONE == errGetByteStride)
            {
                info->allocWidth  = info->width;
                info->allocHeight = info->height;
            }
            else
            {
                mfx_res = MFX_ERR_UNKNOWN;
            }
        }
        else
        {
            mfx_res = MFX_ERR_UNKNOWN;
        }
#else
        int err = m_pGrallocModule->perform(m_pGrallocModule, INTEL_UFO_GRALLOC_MODULE_PERFORM_GET_BO_INFO, handle, info);
        if (0 != err)
        {
            mfx_res = MFX_ERR_UNKNOWN;
        }
#endif // #ifdef MFX_OMX_USE_GRALLOC_1
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAdapter::Lock(buffer_handle_t handle, int32_t width, int32_t height, int usage, mfxU8 **img)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!img)
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
#ifdef MFX_OMX_USE_GRALLOC_1
        gralloc1_rect_t rect;
        rect.left   = 0;
        rect.top    = 0;
        rect.width  = width;
        rect.height = height;

        int32_t err = m_grallocLock(m_pGralloc1Dev,
                                    handle,
                                    (gralloc1_producer_usage_t)usage,
                                    (gralloc1_consumer_usage_t)usage,
                                    &rect,
                                    (void**)img,
                                    -1);

        if (GRALLOC1_ERROR_NONE != err || !(*img))
        {
            mfx_res = MFX_ERR_LOCK_MEMORY;
        }
#else
        android::status_t res = m_pGrallocModule->lock(m_pGrallocModule,
                                                       handle,
                                                       usage,
                                                       0,
                                                       0,
                                                       width,
                                                       height,
                                                       (void**)img);
        if (res != android::OK || !(*img))
        {
            mfx_res = MFX_ERR_LOCK_MEMORY;
        }
#endif
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAdapter::Unlock(buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

#ifdef MFX_OMX_USE_GRALLOC_1
    int32_t releaseFence = -1;
    m_grallocUnlock(m_pGralloc1Dev, handle, &releaseFence);
#else
    m_pGrallocModule->unlock(m_pGrallocModule, handle);
#endif

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAdapter::Alloc(const mfxU16 width, const mfxU16 height, buffer_handle_t *outHandle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!outHandle)
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
#ifdef MFX_OMX_USE_GRALLOC_1
        cros_gralloc_buffer_descriptor *descriptor = new cros_gralloc_buffer_descriptor();

        descriptor->consumer_usage = GRALLOC1_CONSUMER_USAGE_HWCOMPOSER;
        descriptor->producer_usage = GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER;
        descriptor->width          = width;
        descriptor->height         = height;
        descriptor->droid_format   = OMX_INTEL_COLOR_Format_NV12;

        int32_t err = m_grallocAllocate(m_pGralloc1Dev, 1, (gralloc1_buffer_descriptor_t*)descriptor, outHandle);
        if (GRALLOC1_ERROR_NONE != err)
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
#else
        mfxI32 stride = 0;
        mfxI32 err = m_pAllocDev->alloc(m_pAllocDev,
                                        width,
                                        height,
                                        OMX_INTEL_COLOR_Format_NV12,
                                        GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE,
                                        outHandle,
                                        &stride);

        if (err != 0)
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
#endif
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAdapter::Free(const buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

#ifdef MFX_OMX_USE_GRALLOC_1
    int32_t err = m_grallocRelease(m_pGralloc1Dev, handle);
    if (GRALLOC1_ERROR_NONE != err)
    {
        mfx_res = MFX_ERR_INVALID_HANDLE;
    }
#else
    m_pAllocDev->free(m_pAllocDev, handle);
#endif

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

void MfxOmxGrallocAdapter::RegisterBuffer(buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();

#ifndef MFX_OMX_USE_GRALLOC_1
    m_pGrallocModule->registerBuffer(m_pGrallocModule, handle);
#else
    MFX_OMX_UNUSED(handle);
#endif
}

void MfxOmxGrallocAdapter::UnregisterBuffer(buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();

#ifndef MFX_OMX_USE_GRALLOC_1
    m_pGrallocModule->unregisterBuffer(m_pGrallocModule, handle);
#else
    MFX_OMX_UNUSED(handle);
#endif
}

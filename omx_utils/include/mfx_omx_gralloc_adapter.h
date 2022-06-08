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

#ifndef __MFX_OMX_GRALLOC_ADAPTER_H__
#define __MFX_OMX_GRALLOC_ADAPTER_H__

#include "mfx_omx_utils.h"

class MfxOmxGrallocAdapter
{
public:
    static mfxStatus Create(MfxOmxGrallocAdapter **ppGralloc);
    ~MfxOmxGrallocAdapter();

    mfxStatus GetInfo(buffer_handle_t handle, intel_ufo_buffer_details_t *info);

    mfxStatus Lock(buffer_handle_t handle, int32_t width, int32_t height, int usage, mfxU8 **img);
    mfxStatus Unlock(buffer_handle_t handle);

    mfxStatus Alloc(const mfxU16 width, const mfxU16 height, buffer_handle_t *outHandle);
    mfxStatus Free(const buffer_handle_t handle);

    void RegisterBuffer(buffer_handle_t handle);
    void UnregisterBuffer(buffer_handle_t handle);

private:
    MfxOmxGrallocAdapter();
    mfxStatus Init();

private:
    hw_module_t const* m_pModule;

#ifdef MFX_OMX_USE_GRALLOC_1
    gralloc1_device_t* m_pGralloc1Dev;

    GRALLOC1_PFN_GET_FORMAT          m_grallocGetFormat;
    GRALLOC1_PFN_GET_NUM_FLEX_PLANES m_grallocGetNumFlexPlanes;
    GRALLOC1_PFN_GET_BYTE_STRIDE     m_grallocGetByteStride;

#ifdef MFX_OMX_USE_PRIME
    GRALLOC1_PFN_GET_PRIME           m_grallocGetPrime;
#endif
    GRALLOC1_PFN_GET_DIMENSIONS      m_grallocGetDimensions;
    GRALLOC1_PFN_ALLOCATE            m_grallocAllocate;
    GRALLOC1_PFN_RELEASE             m_grallocRelease;
    GRALLOC1_PFN_LOCK                m_grallocLock;
    GRALLOC1_PFN_UNLOCK              m_grallocUnlock;
#else
    alloc_device_t*   m_pAllocDev;
    gralloc_module_t* m_pGrallocModule; // for lock()/unlock()
#endif

    MFX_OMX_CLASS_NO_COPY(MfxOmxGrallocAdapter)
};

#endif // __MFX_OMX_GRALLOC_ADAPTER_H__

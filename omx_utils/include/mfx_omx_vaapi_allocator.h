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

#ifndef __MFX_OMX_VAAPI_ALLOCATOR_H__
#define __MFX_OMX_VAAPI_ALLOCATOR_H__

#if defined(LIBVA_SUPPORT)

#include "mfx_omx_utils.h"
#include "mfx_omx_allocator.h"
#include "mfx_omx_gralloc_adapter.h"

struct vaapiMemId
{
    VASurfaceID* m_pSurface;
    VAImage      m_image;
    mfxU32       m_fourcc;
    const mfxU8* m_key;         // to store HW handle from which this surface has been created (for search already created surface by key)
    mfxU8        m_unused;      // to mark created surface which already unused
    bool         m_bUseBufferDirectly; // if true - we don't need to load data manually from input handle
    mfxU32       m_boName;
};

class MfxOmxVaapiFrameAllocator : public MfxOmxFrameAllocator
{
public:
    static mfxStatus Create(VADisplay dpy, MfxOmxVaapiFrameAllocator **ppAllocator);
    virtual ~MfxOmxVaapiFrameAllocator();

    mfxStatus LoadSurface(const buffer_handle_t handle, bool bIsDecodeTarget, mfxFrameInfo & mfx_info, mfxMemId* pmid);
    mfxStatus RegisterSurface(VASurfaceID surface, mfxMemId* mid, const mfxU8* key, bool bUseBufferDirectly, mfxU32 mfxFourCC, mfxU32 boName);
    mfxStatus MarkUnused(mfxMemId mid);
    mfxStatus FreeExtMID(mfxMemId mid);
    mfxStatus FreeExtMID(buffer_handle_t grallocHandle);

    void FreeSurfaces();

    void RegisterBuffer(buffer_handle_t handle);
    void UnregisterBuffer(buffer_handle_t handle);

protected:
    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr);
    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle);

    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response);
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);

    void upload_yuv_to_surface(unsigned char *newImageBuffer, VASurfaceID surface_id,
                               int picture_width, int picture_height);

protected:
    VADisplay m_dpy;
    MfxOmxGrallocAdapter* m_pGralloc;

private:
    MfxOmxVaapiFrameAllocator();
    mfxStatus Init(VADisplay dpy);

    mfxStatus ConvertGrallocBuffer2MFXMemId(buffer_handle_t handle, bool bIsDecodeTarget, mfxFrameInfo &mfxInfo, mfxMemId* mid);
    mfxStatus MapGrallocBufferToSurface(const mfxU8* handle, bool bIsDecodeTarget, VASurfaceID &surface, mfxFrameInfo &mfxInfo, bool &bUseBufferDirectly, mfxU32 & boName);
    mfxStatus CreateSurfaceFromGralloc(const mfxU8* handle, bool bIsDecodeTarget, VASurfaceID &surface, mfxFrameInfo &mfxInfo, const intel_ufo_buffer_details_t & ufo_details);
    mfxStatus CreateSurface(VASurfaceID &surface, mfxU16 width, mfxU16 height);

    mfxStatus LoadGrallocBuffer(const mfxU8* handle, const VASurfaceID surface);

    mfxStatus TouchSurface(VASurfaceID surface);

private:
    // external buffers
    std::list<vaapiMemId*> m_extMIDs;
    mfxMemId* m_MIDs;
    int m_numMIDs;
    MfxOmxMutex m_mutex;

    MFX_OMX_CLASS_NO_COPY(MfxOmxVaapiFrameAllocator)
};

class MfxOmxGrallocAllocator
{
public:
    static mfxStatus Create(MfxOmxGrallocAllocator **ppAllocator);
    virtual ~MfxOmxGrallocAllocator();

    virtual mfxStatus Alloc(const mfxU16 width, const mfxU16 height, buffer_handle_t & handle);
    virtual mfxStatus Free(const buffer_handle_t handle);
    virtual mfxStatus LockFrame(buffer_handle_t handle, mfxFrameData *ptr);
    virtual mfxStatus UnlockFrame(buffer_handle_t handle, mfxFrameData *ptr);

private:
    MfxOmxGrallocAllocator();
    mfxStatus Init();

protected:

    MfxOmxGrallocAdapter* m_pGralloc;

    MFX_OMX_CLASS_NO_COPY(MfxOmxGrallocAllocator)
};

#endif //#if defined(LIBVA_SUPPORT)

#endif // __MFX_OMX_VAAPI_ALLOCATOR_H__

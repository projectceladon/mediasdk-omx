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

#if defined(LIBVA_SUPPORT)

#include "mfx_omx_utils.h"
#include "mfx_omx_vaapi_allocator.h"

#include "va/va_android.h"

#ifdef MFX_OMX_USE_PRIME
#include "va/va_drmcommon.h"
#endif

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_allocator"

unsigned int ConvertMfxFourccToVAFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
        case MFX_FOURCC_NV12:
            return VA_FOURCC_NV12;
        case MFX_FOURCC_YUY2:
            return VA_FOURCC_YUY2;
        case MFX_FOURCC_YV12:
            return VA_FOURCC_YV12;
        case MFX_FOURCC_RGB4:
            return VA_FOURCC_ARGB;
        case MFX_FOURCC_P8:
            return VA_FOURCC_P208;
        case MFX_FOURCC_P010:
            return VA_FOURCC_P010;
        default:
            return 0;
    }
}

mfxU32 ConvertVAFourccToMfxFormat(unsigned int fourcc)
{
    switch (fourcc)
    {
        case VA_FOURCC_NV12:
            return MFX_FOURCC_NV12;
        case VA_FOURCC_YUY2:
            return MFX_FOURCC_YUY2;
        case VA_FOURCC_YV12:
            return MFX_FOURCC_YV12;
        case VA_FOURCC_RGBA:
        case VA_FOURCC_BGRA:
        case VA_FOURCC_RGBX:
            return MFX_FOURCC_RGB4;
        case VA_FOURCC_P208:
            return MFX_FOURCC_P8;
        case VA_FOURCC_P010:
            return MFX_FOURCC_P010;
        default:
            return 0;
    }
}

unsigned int ConvertGrallocFourccToVAFormat(int fourcc)
{
    switch (fourcc)
    {
        case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
        case HAL_PIXEL_FORMAT_YUV420PackedSemiPlanar_Tiled_INTEL:
        case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
            return VA_FOURCC_NV12;
        case HAL_PIXEL_FORMAT_P010_INTEL:
            return VA_FOURCC_P010;
        case HAL_PIXEL_FORMAT_RGBA_8888:
            return VA_FOURCC_RGBA;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            return VA_FOURCC_RGBX;
        case HAL_PIXEL_FORMAT_BGRA_8888:
            return VA_FOURCC_BGRA;
        default:
            return 0;
    }
}

// We need to sort surface IDs before destroy.
bool compareMid(const vaapiMemId* first, const vaapiMemId* second)
{
    return ((int)(*first->m_pSurface) < (int)(*second->m_pSurface));
}

mfxU32 ConvertVAFourccToVARTFormat(mfxU32 va_fourcc)
{
    switch (va_fourcc)
    {
        case VA_FOURCC_NV12:
        case VA_FOURCC_YV12:
            return VA_RT_FORMAT_YUV420;
        case VA_FOURCC_RGBA:
        case VA_FOURCC_BGRA:
        case VA_FOURCC_RGBX:
            return VA_RT_FORMAT_RGB32;
        case VA_FOURCC_P010:
            return VA_RT_FORMAT_YUV420_10BPP;
        default:
            return 0;
    }
}

MfxOmxVaapiFrameAllocator::MfxOmxVaapiFrameAllocator()
    : m_dpy(NULL)
    , m_pGralloc(NULL)
    , m_MIDs(NULL)
    , m_numMIDs(0)
{
}

mfxStatus MfxOmxVaapiFrameAllocator::Create(VADisplay dpy, MfxOmxVaapiFrameAllocator **ppAllocator)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (ppAllocator)
    {
        MFX_OMX_NEW(*ppAllocator, MfxOmxVaapiFrameAllocator());
        if (*ppAllocator)
        {
            mfx_res = (*ppAllocator)->Init(dpy);
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_DELETE(*ppAllocator);
            }
        }
        else
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }

        MFX_OMX_AUTO_TRACE_P(*ppAllocator);
    }
    else
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::Init(VADisplay dpy)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_dpy = dpy;

    mfx_res = MfxOmxGrallocAdapter::Create(&m_pGralloc);
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("Failed to create MfxOmxGrallocAdapter");
    }

    MFX_OMX_AUTO_TRACE_P(m_pGralloc);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

MfxOmxVaapiFrameAllocator::~MfxOmxVaapiFrameAllocator()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_extMIDs.sort(compareMid);
    while (!m_extMIDs.empty())
    {
        vaapiMemId* pmid = m_extMIDs.back();
        if (pmid)
        {
            if (VA_INVALID_ID != *pmid->m_pSurface)
                vaDestroySurfaces(m_dpy, pmid->m_pSurface, 1);
            free(pmid);
        }
        m_extMIDs.pop_back();
    }
    MFX_OMX_DELETE(m_pGralloc);
}

mfxStatus MfxOmxVaapiFrameAllocator::CheckRequestType(mfxFrameAllocRequest *request)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus sts = MfxOmxFrameAllocator::CheckRequestType(request);
    if (MFX_ERR_NONE != sts)
        return sts;

    if ((request->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)) != 0)
        return MFX_ERR_NONE;
    else
        return MFX_ERR_UNSUPPORTED;
}

mfxStatus MfxOmxVaapiFrameAllocator::RegisterSurface(VASurfaceID surface, mfxMemId* mid, const mfxU8* key, bool bUseBufferDirectly, mfxU32 mfxFourCC, mfxU32 boName)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);

    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxMemId out_mid = NULL;

    MFX_OMX_AUTO_TRACE_I32(surface);

    if (NULL == mid) mfx_res = MFX_ERR_NULL_PTR;

    if (MFX_ERR_NONE == mfx_res)
    {
        // Update fake surface MID with real surface
        if (*mid != NULL && surface != VA_INVALID_ID)
        {
            vaapiMemId* pmid = (vaapiMemId*)(*mid);
            *pmid->m_pSurface = surface;
            pmid->m_key = key;
            pmid->m_boName = boName;
            return mfx_res;
        }

        // Skipping fake MID because real one has already created
        if (*mid != NULL && surface == VA_INVALID_ID )
        {
            return mfx_res;
        }

        // Register MID for fake surface
        if (*mid == NULL && surface == VA_INVALID_ID)
        {
            vaapiMemId* pmid = (vaapiMemId*)malloc(sizeof(vaapiMemId) + sizeof(VASurfaceID));
            if (pmid)
            {
                mfxU8* ddd = (mfxU8*)pmid;
                memset(pmid, 0, sizeof(vaapiMemId) + sizeof(VASurfaceID));
                pmid->m_pSurface = (VASurfaceID*)(mfxU8*)(ddd + sizeof(vaapiMemId));//(VASurfaceID*)(pmid + 1);
                *pmid->m_pSurface = surface;
                pmid->m_fourcc = mfxFourCC;
                pmid->m_boName = boName;
                pmid->m_unused = false;
                pmid->m_key = key;
                pmid->m_bUseBufferDirectly = bUseBufferDirectly;
                out_mid = pmid;
                m_extMIDs.push_back(pmid);
                if (out_mid)
                    *mid = out_mid;
                else
                    mfx_res = MFX_ERR_UNKNOWN;
            }
            else mfx_res = MFX_ERR_NULL_PTR;
            return mfx_res;
        }

        for (std::list<vaapiMemId*>::iterator it = m_extMIDs.begin(); it != m_extMIDs.end(); it++)
        { // if surface already exist
            vaapiMemId* pmid = (*it);

            if (surface == *pmid->m_pSurface)
            {
                out_mid = pmid;
                break;
            }
        }

        if (NULL == out_mid)
        { // add new extMID
            vaapiMemId* pmid = (vaapiMemId*)malloc(sizeof(vaapiMemId) + sizeof(VASurfaceID));
            mfxU8* ddd = (mfxU8*)pmid;
            if (pmid != NULL)
            {
                memset(pmid, 0, sizeof(vaapiMemId) + sizeof(VASurfaceID));
                pmid->m_pSurface = (VASurfaceID*)(mfxU8*)(ddd + sizeof(vaapiMemId));//(VASurfaceID*)(pmid + 1);
                *pmid->m_pSurface = surface;
                pmid->m_fourcc = mfxFourCC;
                pmid->m_boName = boName;
                pmid->m_unused = false;
                pmid->m_key = key;
                pmid->m_bUseBufferDirectly = bUseBufferDirectly;
                out_mid = pmid;
                m_extMIDs.push_back(pmid);
            }
        }

        if (out_mid)
            *mid = out_mid;
        else
            mfx_res = MFX_ERR_UNKNOWN;
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::MarkUnused(mfxMemId mid)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);

    if (NULL == mid) return MFX_ERR_NULL_PTR;

    for (std::list<vaapiMemId*>::iterator it = m_extMIDs.begin(); it != m_extMIDs.end(); it++)
    {
        if ((*it) == mid)
        {
            vaapiMemId* pmid = (*it);
            pmid->m_unused = true;
            return MFX_ERR_NONE;
        }
    }
    return MFX_ERR_NOT_FOUND;
}

mfxStatus MfxOmxVaapiFrameAllocator::FreeExtMID(mfxMemId mid)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    if (NULL == mid) return MFX_ERR_NULL_PTR;

    for (std::list<vaapiMemId*>::iterator it = m_extMIDs.begin(); it != m_extMIDs.end(); it++)
    {
        if ((*it) == mid)
        {
            vaapiMemId* pmid = (*it);
            if (VA_INVALID_ID != *pmid->m_pSurface)
            {
                vaDestroySurfaces(m_dpy, pmid->m_pSurface, 1);
            }
            return MFX_ERR_NONE;
        }
    }
    return MFX_ERR_NOT_FOUND;
}

mfxStatus MfxOmxVaapiFrameAllocator::FreeExtMID(buffer_handle_t grallocHandle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);

    mfxStatus mfx_res = MFX_ERR_NONE;
    bool bIsFound = false;

    if (MFX_ERR_NONE == mfx_res)
    {
        for (std::list<vaapiMemId*>::iterator it = m_extMIDs.begin(); it != m_extMIDs.end(); it++)
        {
            vaapiMemId* pmid = (*it);
            if (pmid->m_key == (const mfxU8*)grallocHandle)
            {
                if (VA_INVALID_ID != *pmid->m_pSurface)
                {
                    vaDestroySurfaces(m_dpy, pmid->m_pSurface, 1);
                }
                free(pmid);
                m_extMIDs.erase(it);
                bIsFound = true;
                break;
            }
        }
    }

    if (!bIsFound) mfx_res = MFX_ERR_NOT_FOUND;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::LoadSurface(const buffer_handle_t handle,
                                                 bool bIsDecodeTarget,
                                                 mfxFrameInfo & mfx_info,
                                                 mfxMemId* pmid)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = ConvertGrallocBuffer2MFXMemId(handle, bIsDecodeTarget, mfx_info, pmid);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::ConvertGrallocBuffer2MFXMemId(buffer_handle_t handle, bool bIsDecodeTarget, mfxFrameInfo &mfxInfo, mfxMemId* pmid)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    VASurfaceID surface;
    mfxU32 boName = 0;

    bool bUseBufferDirectly = true;

    if (handle) mfx_res = MapGrallocBufferToSurface((const mfxU8*)handle, bIsDecodeTarget, surface, mfxInfo, bUseBufferDirectly, boName);
    else surface = VA_INVALID_ID;

    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = RegisterSurface(surface, pmid, (const mfxU8*)handle, bUseBufferDirectly, mfxInfo.FourCC, boName);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::MapGrallocBufferToSurface(const mfxU8* handle, bool bIsDecodeTarget, VASurfaceID &surface, mfxFrameInfo &mfxInfo, bool &bUseBufferDirectly, mfxU32 & boName)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_UNUSED(boName);
    mfxStatus mfx_res = MFX_ERR_NONE;

    surface = VA_INVALID_ID;

    MFX_OMX_AUTO_TRACE_P(handle);

    if (MFX_ERR_NONE == mfx_res && VA_INVALID_ID == surface)
    {
        mfxU32 width = 0;
        mfxU32 height = 0;
        intel_ufo_buffer_details_t info;
        MFX_OMX_ZERO_MEMORY(info);
        *reinterpret_cast<uint32_t*>(&info) = sizeof(info);

        mfx_res = m_pGralloc->GetInfo((buffer_handle_t)handle, &info);
        if (MFX_ERR_NONE == mfx_res)
        {
            MFX_OMX_AUTO_TRACE_U32(info.format);

            if (bIsDecodeTarget)
            {
                bUseBufferDirectly = true;
            }
            else if (   HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL == info.format  // HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL = 0x100
                     || HAL_PIXEL_FORMAT_P010_INTEL == info.format          // HAL_PIXEL_FORMAT_P010_INTEL = 0x110
                     || HAL_PIXEL_FORMAT_RGBX_8888 == info.format           // HAL_PIXEL_FORMAT_RGBX_8888 = 2
                     || HAL_PIXEL_FORMAT_RGBA_8888 == info.format           // HAL_PIXEL_FORMAT_RGBA_8888 = 1
                 )
            {
                bUseBufferDirectly = true;
            }
            else
                bUseBufferDirectly = false;

            width = info.width;
            height = info.height;
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("Failed to get gralloc info");
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            if (bUseBufferDirectly)
                mfx_res = CreateSurfaceFromGralloc(handle, bIsDecodeTarget, surface, mfxInfo, info);
            else
                mfx_res = CreateSurface(surface, width, height);
        }
    }

    if (MFX_ERR_NONE == mfx_res && false == bUseBufferDirectly)
        mfx_res = LoadGrallocBuffer(handle, surface);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::CreateSurfaceFromGralloc(const mfxU8* handle, bool bIsDecodeTarget, VASurfaceID & surface, mfxFrameInfo & mfxInfo, const intel_ufo_buffer_details_t & ufo_details)
{
    MFX_OMX_AUTO_TRACE_FUNC();
#ifdef MFX_OMX_USE_PRIME
    MFX_OMX_UNUSED(handle);
#endif
    mfxStatus mfx_res = MFX_ERR_NONE;
    MFX_OMX_AUTO_TRACE_P(handle);

    const intel_ufo_buffer_details_t & info = ufo_details;
    MFX_OMX_AUTO_TRACE_I32(info.prime);
    MFX_OMX_AUTO_TRACE_I32(info.width);
    MFX_OMX_AUTO_TRACE_I32(info.height);
    MFX_OMX_AUTO_TRACE_I32(info.allocWidth);
    MFX_OMX_AUTO_TRACE_I32(info.allocHeight);
    MFX_OMX_AUTO_TRACE_I32(info.pitch);

    mfxU32 width = bIsDecodeTarget ? info.allocWidth : info.width;
    mfxU32 height = bIsDecodeTarget ? info.allocHeight : info.height;

    mfxU32 va_fourcc = ConvertGrallocFourccToVAFormat(info.format);
    mfxU32 rt_format = ConvertVAFourccToVARTFormat(va_fourcc);

    VASurfaceAttrib attribs[2];
    MFX_OMX_ZERO_MEMORY(attribs);

    VASurfaceAttribExternalBuffers surfExtBuf;
    MFX_OMX_ZERO_MEMORY(surfExtBuf);

    mfxInfo.FourCC = ConvertVAFourccToMfxFormat(va_fourcc);

    surfExtBuf.pixel_format = va_fourcc;
    surfExtBuf.width = width;
    surfExtBuf.height = height;
    surfExtBuf.pitches[0] = info.pitch;
    surfExtBuf.num_planes = 2;
    surfExtBuf.num_buffers = 1;
#ifdef MFX_OMX_USE_PRIME
    surfExtBuf.buffers = (uintptr_t *)&(info.prime);
    surfExtBuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
#else
    surfExtBuf.buffers = (uintptr_t *)&handle;
    surfExtBuf.flags = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
#endif

    attribs[0].type = (VASurfaceAttribType)VASurfaceAttribMemoryType;
    attribs[0].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[0].value.type = VAGenericValueTypeInteger;
#ifdef MFX_OMX_USE_PRIME
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
#else
    attribs[0].value.value.i = VA_SURFACE_ATTRIB_MEM_TYPE_ANDROID_GRALLOC;
#endif

    attribs[1].type = (VASurfaceAttribType)VASurfaceAttribExternalBufferDescriptor;
    attribs[1].flags = VA_SURFACE_ATTRIB_SETTABLE;
    attribs[1].value.type = VAGenericValueTypePointer;
    attribs[1].value.value.p = (void *)&surfExtBuf;

    VAStatus va_res = vaCreateSurfaces(m_dpy, rt_format,
        width, height,
        &surface, 1,
        attribs, MFX_OMX_GET_ARRAY_SIZE(attribs));
    mfx_res = va_to_mfx_status(va_res);

    if (VA_STATUS_SUCCESS != va_res)
    {
        MFX_OMX_LOG_ERROR("Failed vaCreateSurfaces, va_res = 0x%x", va_res);
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        // workaround for a 4kx2k playback performance issue
        if (bIsDecodeTarget && ((mfxInfo.Width >= 2048) || (mfxInfo.Height >= 2048)))
            mfx_res = TouchSurface(surface);
    }

    MFX_OMX_AUTO_TRACE_I32(surface);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::LoadGrallocBuffer(const mfxU8* handle, const VASurfaceID surface)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    intel_ufo_buffer_details_t info;
    MFX_OMX_ZERO_MEMORY(info);
    *reinterpret_cast<uint32_t*>(&info) = sizeof(info);

    mfxU8 *img = NULL;
    bool bIsLocked = false;

    mfx_res = m_pGralloc->GetInfo((buffer_handle_t)handle, &info);
    if (MFX_ERR_NONE == mfx_res)
    {
        MFX_OMX_AUTO_TRACE_I32(info.width);
        MFX_OMX_AUTO_TRACE_I32(info.height);

        mfx_res = m_pGralloc->Lock((buffer_handle_t)handle, info.width, info.height, GRALLOC_USAGE_HW_VIDEO_ENCODER, &img);
        if (MFX_ERR_NONE == mfx_res)
        {
            bIsLocked = true;
        }
        else
        {
            MFX_OMX_AUTO_TRACE_MSG("Failed gralloc lock");
        }
    }
    else
    {
        MFX_OMX_AUTO_TRACE_MSG("Failed to get gralloc info");
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        VAImage image;
        VAStatus va_res = vaDeriveImage(m_dpy, surface, &image);
        if (VA_STATUS_SUCCESS == va_res)
        {
            mfxU8 *pBuffer = NULL;
            mfxU8 *usrptr;
            va_res = vaMapBuffer(m_dpy, image.buf, (void **) &pBuffer);
            if (VA_STATUS_SUCCESS == va_res)
            {
                mfxFrameSurface1 dst, src;

                MFX_OMX_ZERO_MEMORY(dst);
                MFX_OMX_ZERO_MEMORY(src);
                switch (image.format.fourcc)
                {
                case VA_FOURCC_NV12:
                    MFX_OMX_AUTO_TRACE_I32(image.width);
                    MFX_OMX_AUTO_TRACE_I32(image.height);
                    dst.Info.Width = image.width;
                    dst.Info.Height = image.height;
                    dst.Data.Y = pBuffer + image.offsets[0];
                    dst.Data.U = pBuffer + image.offsets[1];
                    dst.Data.V = dst.Data.U + 1;
                    dst.Data.Pitch = (mfxU16)image.pitches[0];
                    MFX_OMX_AUTO_TRACE_I32(dst.Data.Pitch);
                    src.Info.Width = info.width;
                    src.Info.Height = info.height;
                    usrptr = (mfxU8 *)img;
                    src.Data.Y = usrptr;
                    src.Data.U = usrptr + info.pitch * info.height;
                    src.Data.V = src.Data.U + 1;
                    src.Data.Pitch = info.pitch;
                    MFX_OMX_AUTO_TRACE_I32(src.Data.Pitch);

                    mfx_omx_copy_nv12(&dst, &src);

                    break;
                default:
                    va_res = VA_STATUS_ERROR_OPERATION_FAILED;
                    break;
                }
                vaUnmapBuffer(m_dpy, image.buf);
            }
            vaDestroyImage(m_dpy, image.image_id);
        }
        mfx_res = va_to_mfx_status(va_res);
    }

    if (bIsLocked)
    {
        m_pGralloc->Unlock((buffer_handle_t)handle);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::CreateSurface(VASurfaceID &surface, mfxU16 width, mfxU16 height)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus va_res = VA_STATUS_SUCCESS;

    VASurfaceAttrib attrib;

    MFX_OMX_ZERO_MEMORY(attrib);
    attrib.type = VASurfaceAttribPixelFormat;
    attrib.value.type = VAGenericValueTypeInteger;
    attrib.value.value.i = VA_FOURCC_NV12;
    attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;

    va_res = vaCreateSurfaces(
        m_dpy,
        VA_RT_FORMAT_YUV420,
        width, height,
        &surface, 1,
        &attrib, 1);
    mfx_res = va_to_mfx_status(va_res);
    MFX_OMX_AUTO_TRACE_I32(surface);

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::TouchSurface(VASurfaceID surface)
{
    VAImage image;
    unsigned char* buffer;
    VAStatus va_res;

    if (VA_INVALID_ID == surface) return MFX_ERR_UNKNOWN;

    va_res = vaDeriveImage(m_dpy, surface, &image);
    if (VA_STATUS_SUCCESS == va_res)
    {
        va_res = vaMapBuffer(m_dpy, image.buf, (void **) &buffer);
        if (VA_STATUS_SUCCESS == va_res)
        {
            *buffer = 0x0; // can have any value
            vaUnmapBuffer(m_dpy, image.buf);
        }
        vaDestroyImage(m_dpy, image.image_id);
     }

    return MFX_ERR_NONE;
}

mfxStatus MfxOmxVaapiFrameAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus  va_res  = VA_STATUS_SUCCESS;
    unsigned int va_fourcc = 0;
    VASurfaceID* surfaces = NULL;
    VASurfaceAttrib attrib;
    vaapiMemId *vaapi_mids = NULL, *vaapi_mid = NULL;
    mfxMemId* mids = NULL;
    mfxU32 fourcc = request->Info.FourCC;
    mfxU16 surfaces_num = request->NumFrameSuggested, numAllocated = 0, i = 0;
    bool bCreateSrfSucceeded = false;

    memset(response, 0, sizeof(mfxFrameAllocResponse));

    response->reserved[1] = request->Type;

    va_fourcc = ConvertMfxFourccToVAFormat(fourcc);
    if (!va_fourcc || ((VA_FOURCC_NV12 != va_fourcc) &&
                       (VA_FOURCC_YV12 != va_fourcc) &&
                       (VA_FOURCC_YUY2 != va_fourcc) &&
                       (VA_FOURCC_ARGB != va_fourcc) &&
                       (VA_FOURCC_P208 != va_fourcc) &&
                       (VA_FOURCC_P010 != va_fourcc)))
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    if (!surfaces_num)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }
    if (!(request->Type & MFX_OMX_MEMTYPE_GRALLOC))
    {
        /* at the moment: encoder plug-in */
        if (MFX_ERR_NONE == mfx_res)
        {
            surfaces = (VASurfaceID*)calloc(surfaces_num, sizeof(VASurfaceID));
            vaapi_mids = (vaapiMemId*)calloc(surfaces_num, sizeof(vaapiMemId));
            mids = (mfxMemId*)calloc(surfaces_num, sizeof(mfxMemId));
            if ((NULL == surfaces) || (NULL == vaapi_mids) || (NULL == mids)) mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            if (VA_FOURCC_P208 != va_fourcc)
            {
                attrib.type = VASurfaceAttribPixelFormat;
                attrib.value.type = VAGenericValueTypeInteger;
                attrib.value.value.i = va_fourcc;
                attrib.flags = VA_SURFACE_ATTRIB_SETTABLE;

                va_res = vaCreateSurfaces(m_dpy,
                                        VA_RT_FORMAT_YUV420,
                                        request->Info.Width, request->Info.Height,
                                        surfaces,
                                        surfaces_num,
                                        &attrib, 1);
                mfx_res = va_to_mfx_status(va_res);
                bCreateSrfSucceeded = (MFX_ERR_NONE == mfx_res);
            }
            else
            {
                VAContextID context_id = request->AllocId;
                mfxU32 codedbuf_size = (request->Info.Width * request->Info.Height) * 400LL / (16 * 16);

                for (numAllocated = 0; numAllocated < surfaces_num; numAllocated++)
                {
                    VABufferID coded_buf;

                    va_res = vaCreateBuffer(m_dpy,
                                          context_id,
                                          VAEncCodedBufferType,
                                          codedbuf_size,
                                          1,
                                          NULL,
                                          &coded_buf);
                    mfx_res = va_to_mfx_status(va_res);
                    if (MFX_ERR_NONE != mfx_res) break;
                    surfaces[numAllocated] = coded_buf;
                }
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            for (i = 0; i < surfaces_num; ++i)
            {
                vaapi_mid = &(vaapi_mids[i]);
                vaapi_mid->m_fourcc = fourcc;
                vaapi_mid->m_pSurface = &(surfaces[i]);
                mids[i] = vaapi_mid;
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            response->mids = mids;
            response->NumFrameActual = surfaces_num;
        }
        else
        {
            response->mids = NULL;
            response->NumFrameActual = 0;
            if (VA_FOURCC_P208 != va_fourcc)
            {
                if (bCreateSrfSucceeded) vaDestroySurfaces(m_dpy, surfaces, surfaces_num);
            }
            else
            {
                for (i = 0; i < numAllocated; i++)
                    vaDestroyBuffer(m_dpy, surfaces[i]);
            }
            if (mids)
            {
                free(mids);
                mids = NULL;
            }
            if (vaapi_mids) { free(vaapi_mids); vaapi_mids = NULL; }
            if (surfaces) { free(surfaces); surfaces = NULL; }
        }
    }
    else // if (!(request->Type & MFX_OMX_MEMTYPE_GRALLOC))
    {
        /* at the moment: decoder plug-in */
        if (MFX_ERR_NONE == mfx_res && m_MIDs == NULL)
        {
            m_numMIDs = surfaces_num;
            m_MIDs = (mfxMemId*)calloc(m_numMIDs, sizeof(mfxMemId));
            if ((NULL == m_MIDs)) mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            int i = 0;
            for (std::list<vaapiMemId*>::iterator it = m_extMIDs.begin(); it != m_extMIDs.end(); it++)
            { // search for free extMID
                vaapiMemId* pmid = (*it);
                if(i < m_numMIDs)
                {
                    m_MIDs[i] = pmid;
                }
                else
                {
                    mfx_res = MFX_ERR_MEMORY_ALLOC;
                    break;
                }
                i++;
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            response->mids = m_MIDs;
            response->NumFrameActual = surfaces_num;
        }
        else
        {
            response->mids = NULL;
            response->NumFrameActual = 0;
            if (m_MIDs)
            {
                free(m_MIDs);
                m_MIDs = NULL;
            }
        }
    } // if (!(request->Type & MFX_OMX_MEMTYPE_GRALLOC))
    return mfx_res;
}

void MfxOmxVaapiFrameAllocator::FreeSurfaces()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_extMIDs.sort(compareMid);
    while (!m_extMIDs.empty())
    {
        vaapiMemId* pmid = m_extMIDs.back();
        if (pmid)
        {
            if (VA_INVALID_ID != *pmid->m_pSurface)
                vaDestroySurfaces(m_dpy, pmid->m_pSurface, 1);
            free(pmid);
        }
        m_extMIDs.pop_back();
    }
    if (m_MIDs)
    {
        free(m_MIDs);
        m_MIDs = NULL;
    }
}

mfxStatus MfxOmxVaapiFrameAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    vaapiMemId *vaapi_mids = NULL;
    VASurfaceID* surfaces = NULL;
    mfxU32 i = 0;
    bool isBitstreamMemory = false;

    if (!response) return MFX_ERR_NULL_PTR;

    mfxU16 type = (mfxU16)response->reserved[1];
    if (response->mids)
    {
        if (!(type & MFX_OMX_MEMTYPE_GRALLOC))
        {
            vaapi_mids = (vaapiMemId*)(response->mids[0]);
            isBitstreamMemory = (MFX_FOURCC_P8 == vaapi_mids->m_fourcc) ? true : false;
            surfaces = vaapi_mids->m_pSurface;
            for (i = 0; i < response->NumFrameActual; ++i)
            {
                if (MFX_FOURCC_P8 == vaapi_mids[i].m_fourcc) vaDestroyBuffer(m_dpy, surfaces[i]);
            }
            free(vaapi_mids);
            free(response->mids);
            response->mids = NULL;

            if (!isBitstreamMemory)
            {
                vaDestroySurfaces(m_dpy, surfaces, response->NumFrameActual);
                free(surfaces);
            }
        }
        else
        {
            m_extMIDs.sort(compareMid);
            while (!m_extMIDs.empty())
            {
                vaapiMemId* pmid = m_extMIDs.back();
                if (pmid)
                {
                    if (VA_INVALID_ID != *pmid->m_pSurface)
                        vaDestroySurfaces(m_dpy, pmid->m_pSurface, 1);
                    free(pmid);
                }
                m_extMIDs.pop_back();
            }
            if (m_MIDs)
            {
                free(m_MIDs);
                m_MIDs = NULL;
            }
        }
    }
    response->NumFrameActual = 0;
    return MFX_ERR_NONE;
}

mfxStatus MfxOmxVaapiFrameAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus  va_res  = VA_STATUS_SUCCESS;
    vaapiMemId* vaapi_mid = (vaapiMemId*)mid;
    mfxU8* pBuffer = 0;

    if (!vaapi_mid || !(vaapi_mid->m_pSurface)) return MFX_ERR_INVALID_HANDLE;

    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc)   // bitstream processing
    {
        VACodedBufferSegment *coded_buffer_segment;
        va_res =  vaMapBuffer(m_dpy, *(vaapi_mid->m_pSurface), (void **)(&coded_buffer_segment));
        mfx_res = va_to_mfx_status(va_res);
        ptr->Y = (mfxU8*)coded_buffer_segment->buf;
    }
    else   // Image processing
    {
        va_res = vaSyncSurface(m_dpy, *(vaapi_mid->m_pSurface));
        mfx_res = va_to_mfx_status(va_res);

        if (MFX_ERR_NONE == mfx_res)
        {
            va_res = vaDeriveImage(m_dpy, *(vaapi_mid->m_pSurface), &(vaapi_mid->m_image));
            mfx_res = va_to_mfx_status(va_res);
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            va_res = vaMapBuffer(m_dpy, vaapi_mid->m_image.buf, (void **) &pBuffer);
            mfx_res = va_to_mfx_status(va_res);
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            switch (vaapi_mid->m_image.format.fourcc)
            {
            case VA_FOURCC_NV12:
                if (vaapi_mid->m_fourcc == MFX_FOURCC_NV12)
                {
                    ptr->Pitch = (mfxU16)vaapi_mid->m_image.pitches[0];
                    ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
                    ptr->U = pBuffer + vaapi_mid->m_image.offsets[1];
                    ptr->V = ptr->U + 1;
                }
                else mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            case VA_FOURCC_P010:
                if (vaapi_mid->m_fourcc == MFX_FOURCC_P010)
                {
                    ptr->Pitch = (mfxU16)vaapi_mid->m_image.pitches[0];
                    ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
                    ptr->U = pBuffer + vaapi_mid->m_image.offsets[1];
                    ptr->V = ptr->U + 1;
                }
                else mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            case VA_FOURCC_YV12:
                if (vaapi_mid->m_fourcc == MFX_FOURCC_YV12)
                {
                    ptr->Pitch = (mfxU16)vaapi_mid->m_image.pitches[0];
                    ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
                    ptr->V = pBuffer + vaapi_mid->m_image.offsets[1];
                    ptr->U = pBuffer + vaapi_mid->m_image.offsets[2];
                }
                else mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            case VA_FOURCC_YUY2:
                if (vaapi_mid->m_fourcc == MFX_FOURCC_YUY2)
                {
                    ptr->Pitch = (mfxU16)vaapi_mid->m_image.pitches[0];
                    ptr->Y = pBuffer + vaapi_mid->m_image.offsets[0];
                    ptr->U = ptr->Y + 1;
                    ptr->V = ptr->Y + 3;
                }
                else mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            case VA_FOURCC_ARGB:
            case VA_FOURCC_ABGR:
                if (vaapi_mid->m_fourcc == MFX_FOURCC_RGB4)
                {
                    ptr->Pitch = (mfxU16)vaapi_mid->m_image.pitches[0];
                    ptr->B = pBuffer + vaapi_mid->m_image.offsets[0];
                    ptr->G = ptr->B + 1;
                    ptr->R = ptr->B + 2;
                    ptr->A = ptr->B + 3;
                }
                else mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            default:
                mfx_res = MFX_ERR_LOCK_MEMORY;
                break;
            }
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxVaapiFrameAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    vaapiMemId* vaapi_mid = (vaapiMemId*)mid;
    MfxOmxAutoLock lock(m_mutex);

    if (!vaapi_mid || !(vaapi_mid->m_pSurface)) return MFX_ERR_INVALID_HANDLE;

    if (MFX_FOURCC_P8 == vaapi_mid->m_fourcc)   // bitstream processing
    {
        vaUnmapBuffer(m_dpy, *(vaapi_mid->m_pSurface));
    }
    else  // Image processing
    {
        vaUnmapBuffer(m_dpy, vaapi_mid->m_image.buf);
        vaDestroyImage(m_dpy, vaapi_mid->m_image.image_id);

        if (NULL != ptr)
        {
            ptr->Pitch = 0;
            ptr->Y     = NULL;
            ptr->U     = NULL;
            ptr->V     = NULL;
            ptr->A     = NULL;
        }
    }
    return MFX_ERR_NONE;
}

mfxStatus MfxOmxVaapiFrameAllocator::GetFrameHDL(mfxMemId mid, mfxHDL *handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);

    vaapiMemId* vaapi_mid = (vaapiMemId*)mid;

    if (!handle || !vaapi_mid || !(vaapi_mid->m_pSurface)) return MFX_ERR_INVALID_HANDLE;

    *handle = vaapi_mid->m_pSurface; //VASurfaceID* <-> mfxHDL
    return MFX_ERR_NONE;
}

void MfxOmxVaapiFrameAllocator::upload_yuv_to_surface(unsigned char *newImageBuffer, VASurfaceID surface_id, int picture_width, int picture_height)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    VAImage surface_image;
    VAStatus va_status;
    void *surface_p = NULL;
    unsigned char *y_src, *u_src, *v_src;
    unsigned char *y_dst, *u_dst, *v_dst;
    int row;
    va_status = vaDeriveImage(m_dpy, surface_id, &surface_image);

    if(va_status != VA_STATUS_SUCCESS) return ;
    int y_size = picture_width * picture_height;
    int u_size = (picture_width >> 1) * (picture_height >> 1);

    vaMapBuffer(m_dpy, surface_image.buf, &surface_p);
    y_src = newImageBuffer;
    u_src = newImageBuffer + y_size; /* UV offset for NV12 */
    v_src = newImageBuffer + y_size + u_size;

    y_dst = (unsigned char *)surface_p + surface_image.offsets[0];
    u_dst = (unsigned char *)surface_p + surface_image.offsets[1]; /* UV offset for NV12 */
    v_dst = (unsigned char *)surface_p + surface_image.offsets[2];

    /* Y plane */
    for (row = 0; row < surface_image.height; row++) {
        memcpy_s(y_dst, surface_image.width, y_src, surface_image.width);
        y_dst += surface_image.pitches[0];
        y_src += picture_width;
    }

    for (row = 0; row < surface_image.height / 2; row++)
    {
        memcpy_s(u_dst, surface_image.width, u_src, surface_image.width);
        u_dst += surface_image.pitches[1];
        u_src += picture_width;
    }

    vaUnmapBuffer(m_dpy, surface_image.buf);
    vaDestroyImage(m_dpy, surface_image.image_id);
}

void MfxOmxVaapiFrameAllocator::RegisterBuffer(buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_pGralloc->RegisterBuffer(handle);
}

void MfxOmxVaapiFrameAllocator::UnregisterBuffer(buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_pGralloc->UnregisterBuffer(handle);
}

MfxOmxGrallocAllocator::MfxOmxGrallocAllocator()
    : m_pGralloc(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

mfxStatus MfxOmxGrallocAllocator::Create(MfxOmxGrallocAllocator **ppAllocator)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (ppAllocator)
    {
        MFX_OMX_NEW(*ppAllocator, MfxOmxGrallocAllocator());
        if (*ppAllocator)
        {
            mfx_res = (*ppAllocator)->Init();
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_DELETE(*ppAllocator);
            }
        }
        else
        {
            mfx_res = MFX_ERR_MEMORY_ALLOC;
        }

        MFX_OMX_AUTO_TRACE_P(*ppAllocator);
    }
    else
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAllocator::Init()
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = MfxOmxGrallocAdapter::Create(&m_pGralloc);
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("Failed to create MfxOmxGrallocAdapter");
    }

    MFX_OMX_AUTO_TRACE_P(m_pGralloc);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

MfxOmxGrallocAllocator::~MfxOmxGrallocAllocator()
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_DELETE(m_pGralloc);
}

mfxStatus MfxOmxGrallocAllocator::Alloc(const mfxU16 width, const mfxU16 height, buffer_handle_t & outHandle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_I32(width);
    MFX_OMX_AUTO_TRACE_I32(height);

    mfx_res = m_pGralloc->Alloc(width, height, &outHandle);
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_AUTO_TRACE_MSG("Failed gralloc allocate");
    }

    MFX_OMX_AUTO_TRACE_P(outHandle);
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAllocator::Free(const buffer_handle_t handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_P(handle);

    if (handle)
    {
        mfx_res = m_pGralloc->Free(handle);
    }
    else
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAllocator::LockFrame(buffer_handle_t handle, mfxFrameData *ptr)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    intel_ufo_buffer_details_t info;
    MFX_OMX_ZERO_MEMORY(info);

    if (!ptr)
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        *reinterpret_cast<uint32_t*>(&info) = sizeof(info);

        mfx_res = m_pGralloc->GetInfo(handle, &info);
        if (MFX_ERR_NONE != mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("Failed to get gralloc info");
        }
    }

    mfxU8 *img = NULL;
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = m_pGralloc->Lock(handle, info.width, info.height, GRALLOC_USAGE_SW_READ_MASK | GRALLOC_USAGE_SW_WRITE_MASK, &img);
        if (MFX_ERR_NONE != mfx_res)
        {
            MFX_OMX_AUTO_TRACE_MSG("Failed gralloc lock");
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        ptr->Pitch = info.pitch;
        ptr->Y     = img;
        ptr->U     = img + info.allocHeight * info.pitch;
        ptr->V     = ptr->U + 1;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

mfxStatus MfxOmxGrallocAllocator::UnlockFrame(buffer_handle_t handle, mfxFrameData *ptr)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    m_pGralloc->Unlock(handle);

    if (ptr)
    {
        ptr->Pitch = 0;
        ptr->Y     = NULL;
        ptr->U     = NULL;
        ptr->V     = NULL;
        ptr->A     = NULL;
    }

    return MFX_ERR_NONE;
}

#endif // #if defined(LIBVA_SUPPORT)

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

#include "mfx_omx_srf_ibuf.h"
#include "mfx_omx_vaapi_allocator.h"
#include "mfx_omx_utils.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_srf_ibuf"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_IS_SURFACE_LOADED(_surface) \
    ((_surface).Data.MemId || (_surface).Data.Y || (_surface).Data.R)

#define MFX_OMX_IS_COPY_NEEDED(_mem_type, _input_info, _mfx_info) \
    ((MODE_LOAD_SWMEM == _mem_type) && \
    (((_input_info).Width < (_mfx_info.Width)) || \
    ((_input_info).Height < (_mfx_info.Height))))

/*------------------------------------------------------------------------------*/

MfxOmxInputSurfacesPool::MfxOmxInputSurfacesPool(mfxStatus &sts):
    MfxOmxInputRefBuffersPool<OMX_BUFFERHEADERTYPE,mfxFrameSurface1>(sts),
    m_bInitialized(false),
    m_bEOS(false),
    m_pDevice(NULL),
    m_inputDataMode(MODE_LOAD_SWMEM),
    m_blackFrame(NULL),
    m_dbg_file(NULL)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MFX_OMX_ZERO_MEMORY(m_InputFramesInfo);
    MFX_OMX_ZERO_MEMORY(m_MfxFramesInfo);
}

/*------------------------------------------------------------------------------*/

MfxOmxInputSurfacesPool::~MfxOmxInputSurfacesPool(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (MFX_ERR_NONE == mfx_res)
    {
        m_bEOS = false;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = MfxOmxInputRefBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::Reset();
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::Init(
    mfxFrameInfo* pInputFramesInfo,
    mfxFrameInfo* pMfxInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (m_bInitialized) mfx_res = MFX_ERR_ABORTED;
    if ((MFX_ERR_NONE == mfx_res) && (!pInputFramesInfo || !pMfxInfo))
    {
        mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        m_InputFramesInfo = *pInputFramesInfo;
        m_MfxFramesInfo = *pMfxInfo;

        MFX_OMX_AT__mfxFrameInfo(m_InputFramesInfo);
        MFX_OMX_AT__mfxFrameInfo(m_MfxFramesInfo);

        if ((MFX_FOURCC_NV12 != m_MfxFramesInfo.FourCC) &&
            (MFX_FOURCC_YUY2 != m_MfxFramesInfo.FourCC))
        {
            mfx_res = MFX_ERR_ABORTED;
        }
        if ((MFX_ERR_NONE == mfx_res) &&
            (MFX_CHROMAFORMAT_YUV420 != m_MfxFramesInfo.ChromaFormat) &&
            (MFX_CHROMAFORMAT_YUV422 != m_MfxFramesInfo.ChromaFormat))
        {
            mfx_res = MFX_ERR_ABORTED;
        }
    }
    if (MFX_ERR_NONE == mfx_res) m_bInitialized = true;
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

void MfxOmxInputSurfacesPool::Close(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    m_bInitialized = false;
    Reset();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::SetMfxDevice(MfxOmxDev* dev)
{
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (dev)
        m_pDevice = dev;
    else
        mfx_res = MFX_ERR_NULL_PTR;

    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::SetMode(const mfxU32 mode)
{
    m_inputDataMode = mode;

    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::PrepareSurface(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = NULL;
    mfxU8* pVideo16 = NULL;
    mfxU16 nPitch = 0, align = 1;

    if (!m_bInitialized) mfx_res = MFX_ERR_NOT_INITIALIZED;
    if ((MFX_ERR_NONE == mfx_res) && !pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pBufInfo = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;
        if (!pBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        MFX_OMX_ZERO_MEMORY(pBufInfo->sSurface);
        pBufInfo->sSurface.Info = m_MfxFramesInfo;
        MFX_OMX_AUTO_TRACE_I32(pBufInfo->sSurface.Info.FourCC);

        nPitch = (mfxU16)(MFX_OMX_MEM_ALIGN(m_MfxFramesInfo.Width, align));

        if (MFX_OMX_IS_COPY_NEEDED(m_inputDataMode, m_InputFramesInfo, m_MfxFramesInfo))
        {
            switch(m_MfxFramesInfo.FourCC)
            {
                case MFX_FOURCC_NV12:
                    pVideo16 = (mfxU8*)calloc(3*nPitch*m_MfxFramesInfo.Height/2, sizeof(mfxU8));
                    if (pVideo16)
                    {
                        pBufInfo->sSurface.Data.Y = pVideo16;
                        pBufInfo->sSurface.Data.UV = pVideo16 + nPitch * m_MfxFramesInfo.Height;
                        pBufInfo->sSurface.Data.Pitch = nPitch;
                    }
                    else mfx_res = MFX_ERR_MEMORY_ALLOC;
                    break;
                case MFX_FOURCC_YUY2:
                    pVideo16 = (mfxU8*)calloc(2*nPitch*m_MfxFramesInfo.Height, sizeof(mfxU8));
                    if (pVideo16)
                    {
                        pBufInfo->sSurface.Data.Y = pVideo16;
                        pBufInfo->sSurface.Data.U = pVideo16 + 1;
                        pBufInfo->sSurface.Data.V = pVideo16 + 3;
                        pBufInfo->sSurface.Data.Pitch = 2*nPitch;
                    }
                    else mfx_res = MFX_ERR_MEMORY_ALLOC;
                    break;
                default:
                    // we should not be here: we checked that we work with suitable color format...
                    mfx_res = MFX_ERR_UNSUPPORTED;
                    break;
            };
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::FreeSurface(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = NULL;
    mfxU8* pVideo16 = NULL;

    if (!pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pBufInfo = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;
        if (!pBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        if (MFX_OMX_IS_COPY_NEEDED(m_inputDataMode, m_InputFramesInfo, m_MfxFramesInfo))
        {
            pVideo16 = pBufInfo->sSurface.Data.Y;
            MFX_OMX_FREE(pVideo16);

            MFX_OMX_ZERO_MEMORY(pBufInfo->sSurface);
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::UseBuffer(
    OMX_BUFFERHEADERTYPE* pBuffer,
    MfxOmxInputConfig config)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = LoadSurface(pBuffer, config);
    if (pBuffer)
    {
         m_bEOS = (pBuffer->nFlags & OMX_BUFFERFLAG_EOS);
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = MfxOmxInputBuffersPool<OMX_BUFFERHEADERTYPE>::UseBuffer(pBuffer);
    }
    else if (m_bEOS)
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
        mfx_res = MFX_ERR_NONE;
    }
    else if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == mfx_res)
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::IsBufferLocked(
    OMX_BUFFERHEADERTYPE* pBuffer,
    bool& bIsLocked)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = MfxOmxGetInputBufferInfo<OMX_BUFFERHEADERTYPE>(pBuffer);

    bIsLocked = false;
    if (pBufInfo)
    {
        if (MFX_OMX_IS_SURFACE_LOADED(pBufInfo->sSurface))
        {
            if (pBufInfo->sSurface.Data.Locked) bIsLocked = true;
        }
    }
    else mfx_res = MFX_ERR_NULL_PTR;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::LoadSurface(
    OMX_BUFFERHEADERTYPE* pBuffer,
    MfxOmxInputConfig& config)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = NULL;
    mfxU8* data = NULL;

    if (!m_bInitialized) mfx_res = MFX_ERR_NOT_INITIALIZED;
    if ((MFX_ERR_NONE == mfx_res) && !pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pBufInfo = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;
        if (!pBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        data = (mfxU8*)pBuffer->pBuffer;
        if (data)
        {
            data += pBuffer->nOffset;
        }
        else mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
    }

    mfxFrameSurface1 & surface = pBufInfo->sSurface;
    if (MFX_ERR_NONE == mfx_res)
    {
        switch (m_inputDataMode)
        {
            case MODE_LOAD_MDBUF:
            case MODE_LOAD_OPAQUE:
                mfx_res = LoadSurfaceHW(data, pBuffer->nFilledLen, &surface);
                break;

            case MODE_LOAD_SWMEM:
                mfx_res = LoadSurfaceSW(data, pBuffer->nFilledLen, &surface);
                break;

            default:
                mfx_res = MFX_ERR_UNSUPPORTED;
                break;
        }
    }
    if (m_dbg_file && MFX_ERR_NONE == mfx_res)
    {
        if (surface.Data.MemId)
        {
            MFX_OMX_AUTO_TRACE_P(surface.Data.MemId);
            MfxOmxVaapiFrameAllocator *pAllocator = (MfxOmxVaapiFrameAllocator*)m_pDevice->GetFrameAllocator();
            if (pAllocator)
            {
                mfx_res = pAllocator->Lock(pAllocator->pthis, surface.Data.MemId, &(surface.Data));
                if (MFX_ERR_NONE == mfx_res)
                {
                    switch(surface.Info.FourCC)
                    {
                        case MFX_FOURCC_NV12:
                            mfx_omx_dump_YUV_from_NV12_data(m_dbg_file, &(surface.Data), &(m_MfxFramesInfo), surface.Data.Pitch);
                            break;
                        case MFX_FOURCC_RGB4:
                            mfx_omx_dump_RGB_from_RGB4_data(m_dbg_file, &(surface.Data), &(m_MfxFramesInfo), surface.Data.Pitch);
                            break;
                        default:
                            break;
                    }
                    mfx_res = pAllocator->Unlock(pAllocator->pthis, surface.Data.MemId, &(surface.Data));
                }
            }
        }
        else
        {
            switch(surface.Info.FourCC)
            {
                case MFX_FOURCC_NV12:
                    mfx_omx_dump_YUV_from_NV12_data(m_dbg_file, &(surface.Data), &(m_MfxFramesInfo), surface.Data.Pitch);
                    break;
                default:
                    break;
            }
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        surface.Data.TimeStamp = OMX2MFX_TIME(pBuffer->nTimeStamp);

        pBufInfo->config = config;
        /** @todo Need to convert some other parameters like PicStruct. */
        //m_MfxSurface.Info.PicStruct =
    }
    if (MFX_ERR_NONE != mfx_res)
    {
        MFX_OMX_DELETE(config.control);
        MFX_OMX_DELETE(config.mfxparams);
    }
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::LoadSurfaceSW(mfxU8 *data, mfxU32 length, mfxFrameSurface1* srf)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    mfxU16  nPitch = 0;
    mfxU16  nOPitch = 0;
    mfxU16  nWidth = 0, nHeight = 0;
    mfxU16  nOWidth = 0, nOHeight = 0;
    mfxU16  nCropX = 0, nCropY  = 0;
    mfxU16  nCropW = 0, nCropH  = 0;

    if (!m_bInitialized) mfx_res = MFX_ERR_NOT_INITIALIZED;
    if ( MFX_ERR_NONE == mfx_res && (!data || !length || !srf) ) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        nOWidth  = m_InputFramesInfo.Width;
        nOHeight = m_InputFramesInfo.Height;
        nOPitch  = nOWidth;

        nWidth   = m_MfxFramesInfo.Width;
        nHeight  = m_MfxFramesInfo.Height;
        nCropX   = m_MfxFramesInfo.CropX;
        nCropY   = m_MfxFramesInfo.CropY;
        nCropW   = m_MfxFramesInfo.CropW;
        nCropH   = m_MfxFramesInfo.CropH;
        nPitch   = srf->Data.Pitch;

        switch(m_MfxFramesInfo.FourCC)
        {
        case MFX_FOURCC_NV12:
            if (length >= (mfxU32)(3*nOWidth*nOHeight/2))
            {
                if (!MFX_OMX_IS_COPY_NEEDED(m_inputDataMode, m_InputFramesInfo, m_MfxFramesInfo))
                {
                    srf->Data.Y  = data;
                    srf->Data.UV = data + nOWidth * nOHeight;
                    srf->Data.Pitch = nOPitch;
                }
                else
                {
                    mfxU16 i = 0;
                    mfxU8* Y  = data;
                    mfxU8* UV = data + nOPitch * nOHeight;

                    for (i = 0; i < nCropH/2; ++i)
                    {
                        // copying Y
                        uint8_t *src = Y + nCropX + (nCropY + i)*nOPitch;
                        uint8_t *dst = srf->Data.Y + nCropX + (nCropY + i)*nPitch;
                        std::copy(src, src + nCropW, dst);

                        // copying UV
                        src = UV + nCropX + (nCropY/2 + i)*nOPitch;
                        dst = srf->Data.UV + nCropX + (nCropY/2 + i)*nPitch;
                        std::copy(src, src + nCropW, dst);
                    }
                    for (i = nCropH/2; i < nCropH; ++i)
                    {
                        // copying Y (remained data)
                        uint8_t *src = Y + nCropX + (nCropY + i)*nOPitch;
                        uint8_t *dst = srf->Data.Y + nCropX + (nCropY + i)*nPitch;
                        std::copy(src, src + nCropW, dst);
                    }
                }
            }
            else mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
            break;
        case MFX_FOURCC_YUY2:
            if (length >= (mfxU32)(2*nOWidth*nOHeight))
            {
                if (!MFX_OMX_IS_COPY_NEEDED(m_inputDataMode, m_InputFramesInfo, m_MfxFramesInfo))
                {
                    srf->Data.Y = data;
                    srf->Data.U = data + 1;
                    srf->Data.V = data + 3;
                    srf->Data.Pitch = 2*nOPitch;
                }
                else
                {
                    mfxU16 i = 0;
                    mfxU8* Y = data;

                    for (i = 0; i < nCropH; ++i)
                    {
                        uint8_t *src = Y + nCropX + (nCropY + i) * 2 * nOPitch;
                        uint8_t *dst = srf->Data.Y + nCropX + (nCropY + i) * 2 * nPitch;
                        std::copy(src, src + 2 * nCropW, dst);
                    }
                }
            }
            else mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
            break;
        default:
            // will not occur
            mfx_res = MFX_ERR_UNSUPPORTED;
            break;
        }
    }
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::LoadSurfaceHW(mfxU8 *data, mfxU32 length, mfxFrameSurface1* mfxSurface)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxVaapiFrameAllocator *allocator = NULL;
    mfxMemId mid = NULL;
    MetadataBuffer buffer;

    if (!m_bInitialized) mfx_res = MFX_ERR_NOT_INITIALIZED;
    if (MFX_ERR_NONE == mfx_res && (!data || !length || !mfxSurface)) mfx_res = MFX_ERR_NULL_PTR;

    if (MFX_ERR_NONE == mfx_res)
    {
        allocator = (MfxOmxVaapiFrameAllocator*)m_pDevice->GetFrameAllocator();
        if (!allocator) mfx_res = MFX_ERR_NULL_PTR;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        MFX_OMX_ZERO_MEMORY(buffer);
        mfx_res = mfx_omx_get_metadatabuffer_info(data, length, &buffer);
        MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "gralloc handle %p", buffer.handle);
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        if (MfxMetadataBufferTypeGrallocSource == buffer.type)
        {
            mfx_res = allocator->LoadSurface((buffer_handle_t)buffer.handle, false, m_MfxFramesInfo, &mid);
        }
        else if (MfxMetadataBufferTypeNativeHandleSource == buffer.type)
        {
            allocator->RegisterBuffer(buffer.handle);
            mfx_res = allocator->LoadSurface((buffer_handle_t)buffer.handle, false, m_MfxFramesInfo, &mid);
        }
        else mfx_res = MFX_ERR_UNSUPPORTED;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        MFX_OMX_ZERO_MEMORY(mfxSurface->Data);
        mfxSurface->Info = m_MfxFramesInfo;
        mfxSurface->Data.MemId = mid;

        MFX_OMX_AUTO_TRACE_P(mid);
    }

    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxInputSurfacesPool::ReleaseBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = NULL;

    MFX_OMX_AUTO_TRACE_P(pBuffer);
    if (!m_bInitialized) mfx_res = MFX_ERR_NOT_INITIALIZED;
    if ((MFX_ERR_NONE == mfx_res) && !pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pBufInfo = (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate;
        if (!pBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        MfxOmxVaapiFrameAllocator* allocator = NULL;

        allocator = (MfxOmxVaapiFrameAllocator*)m_pDevice->GetFrameAllocator();
        if (NULL != allocator)
        {
            if (pBufInfo->sSurface.Data.MemId)
            {
                buffer_handle_t handle = GetGrallocHandle(pBuffer);
                if (handle != m_blackFrame) mfx_res = allocator->FreeExtMID(handle); // don't release vaSurface for the black frame

                MfxMetadataBufferType type = GetMetadataType(pBuffer);
                if (MfxMetadataBufferTypeNativeHandleSource == type)
                {
                    allocator->UnregisterBuffer(handle);
                }
            }
        }
        else
        {
            mfx_res = MFX_ERR_NULL_PTR;
        }
        if (pBufInfo->config.control) // free payload data
        {
            if (pBufInfo->config.control->payload)
            {
                MFX_OMX_DELETE(pBufInfo->config.control->payload->Data);
            }
            MFX_OMX_DELETE(pBufInfo->config.control->payload);
        }
        MFX_OMX_DELETE(pBufInfo->config.control);
        MFX_OMX_DELETE(pBufInfo->config.mfxparams);
    }
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxInputSurfacesPool::IsRepeatedFrame(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    bool bIsRepeatedFrame = false;
    //MfxOmxBufferInfo* pBufInfo = NULL;
    MetadataBuffer metaBuffer;

    if (m_inputDataMode != MODE_LOAD_SWMEM)
    {
        OMX_BUFFERHEADERTYPE* pBuffer = GetCurrentBuffer();
        MFX_OMX_AUTO_TRACE_P(pBuffer);
        if (!pBuffer) mfx_res = MFX_ERR_NULL_PTR;
        if (MFX_ERR_NONE == mfx_res)
        {
            MFX_OMX_ZERO_MEMORY(metaBuffer);
            mfx_res = mfx_omx_get_metadatabuffer_info(pBuffer->pBuffer + pBuffer->nOffset, pBuffer->nFilledLen, &metaBuffer);
        }
        // TBD
    }

    MFX_OMX_AUTO_TRACE_I32(bIsRepeatedFrame);
    return bIsRepeatedFrame;
}

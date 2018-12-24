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

#include "mfx_omx_srf_pool.h"
#include "mfx_omx_vaapi_allocator.h"

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_srf_pool"

/*------------------------------------------------------------------------------*/

MfxOmxSurfacesPool::MfxOmxSurfacesPool(mfxU32 CodecId, mfxStatus &sts):
    MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>(sts),
    m_codecId(CodecId),
    m_nPitch(0),
    m_pFrameAllocator(NULL),
    m_pDevice(NULL),
    m_bOnFlySurfacesAllocation(false),
    m_bANWBufferInMetaData(false),
    m_latestTS(0),
    m_dbg_file(NULL),
    m_maxErrorCount(1)
{
    MFX_OMX_AUTO_TRACE_FUNC();
}

/*------------------------------------------------------------------------------*/

MfxOmxSurfacesPool::~MfxOmxSurfacesPool(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    while (false == m_errorList.empty())
    {
        delete m_errorList.front();
        m_errorList.pop_front();
    }

    while (!m_BufferInfoPool.empty())
    {
        MfxOmxBufferInfo* pAddBufInfo = m_BufferInfoPool.front();
        m_BufferInfoPool.pop_front();
        if (pAddBufInfo)
        {
            if (pAddBufInfo->pAnwBuffer)
            {
                pAddBufInfo->pAnwBuffer->decStrong(NULL);
                pAddBufInfo->pAnwBuffer = NULL;
            }
            if (!pAddBufInfo->bUsed)
            {
                MFX_OMX_FREE(pAddBufInfo);
            }
        }
    }
    while (!m_SwapPool.empty())
    {
        MfxOmxBufferSwapPair* ptr = m_SwapPool.front();
        m_SwapPool.pop_front();
        MFX_OMX_FREE(ptr);
    }
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::Close(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    while (!m_BufferInfoPool.empty())
    {
        MfxOmxBufferInfo* pAddBufInfo = m_BufferInfoPool.front();
        m_BufferInfoPool.pop_front();
        if (pAddBufInfo)
        {
            if (pAddBufInfo->pAnwBuffer)
            {
                pAddBufInfo->pAnwBuffer->decStrong(NULL);
                pAddBufInfo->pAnwBuffer = NULL;
            }
            if (!pAddBufInfo->bUsed)
            {
                MFX_OMX_FREE(pAddBufInfo);
            }
        }
    }
    while (!m_SwapPool.empty())
    {
        MfxOmxBufferSwapPair* ptr = m_SwapPool.front();
        m_SwapPool.pop_front();
        MFX_OMX_FREE(ptr);
    }

    mfx_res = Reset();
    if (MFX_ERR_NONE == mfx_res)
    {
        m_nPitch = 0;
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    m_latestTS = 0;
    MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::Reset();

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::PrepareSurface(OMX_BUFFERHEADERTYPE* pBuffer, mfxFrameInfo* pFrameInfo, mfxMemId MemId)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pAddBufInfo = NULL;

    if (!pBuffer || !pFrameInfo) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        pAddBufInfo = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;
        if (!pAddBufInfo) mfx_res = MFX_ERR_NULL_PTR;
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        pAddBufInfo->sSurface.Info = *pFrameInfo;
        if (!MemId)
        {
            if (!m_nPitch) m_nPitch = GetPitch(pFrameInfo);
            pAddBufInfo->sSurface.Data.Pitch = m_nPitch;
            switch (pFrameInfo->FourCC)
            {
            case MFX_FOURCC_NV12:
                pAddBufInfo->sSurface.Data.Y = pBuffer->pBuffer;
                pAddBufInfo->sSurface.Data.UV = pBuffer->pBuffer + pAddBufInfo->sSurface.Data.Pitch * pFrameInfo->Height;
                break;
            case MFX_FOURCC_P010:
                pAddBufInfo->sSurface.Data.Y16 = (mfxU16*) pBuffer->pBuffer;
                pAddBufInfo->sSurface.Data.U16 = (mfxU16*) pBuffer->pBuffer + pAddBufInfo->sSurface.Data.Pitch * pFrameInfo->Height;
                pAddBufInfo->sSurface.Data.V = (mfxU8*) (pAddBufInfo->sSurface.Data.U16 + 1);
                break;
            };
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::CheckBufferForLog(MfxOmxRing<OMX_BUFFERHEADERTYPE*>& buffersList, MfxOmxBufferInfo* pBuffInfo, bool& bUsed)
{
    mfxStatus mfx_res = MFX_ERR_NONE;
    OMX_BUFFERHEADERTYPE* pBufferForLog = NULL;
    const mfxU32 buffers_num = buffersList.GetItemsCount();
    for (mfxU32 i = 0; i < buffers_num; ++i)
    {
        if (buffersList.Get(&pBufferForLog))
        {
            MfxOmxBufferInfo* pAddBufInfoForLog = MfxOmxGetOutputBufferInfo(pBufferForLog);
            if (!buffersList.Add(&pBufferForLog))
            {
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBufferForLog, MFX_ERR_UNKNOWN);
                mfx_res = MFX_ERR_UNKNOWN;
            }
            if (pBuffInfo == pAddBufInfoForLog) bUsed = true;
        }
    }
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::FindAvailableSurface(OMX_BUFFERHEADERTYPE** ppBuffer, MfxOmxBufferInfo** ppAddBufInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MfxOmxAutoLock lock(m_mutex);

    bool bFound = false;
    for (std::list<MfxOmxBufferInfo*>::iterator it3 = m_BufferInfoPool.begin(); it3 != m_BufferInfoPool.end(); it3++)
    {
        if ((*it3) && (!(*it3)->sSurface.Data.Locked))
        {
            bool bUsed = false;

            // Check for m_pCurrentBufferUnlocked
            if ((*it3) == MfxOmxGetOutputBufferInfo(m_pCurrentBufferUnlocked)) bUsed = true;

            // Check for m_pCurrentBufferToBeSent
            if ((*it3) == MfxOmxGetOutputBufferInfo(m_pCurrentBufferToBeSent)) bUsed = true;

            mfx_res = CheckBufferForLog(m_BuffersLocked, (*it3), bUsed);

            if (MFX_ERR_NONE == mfx_res)
            {
                mfx_res = CheckBufferForLog(m_BuffersUnlocked, (*it3), bUsed);
            }
            if (MFX_ERR_NONE == mfx_res)
            {
                mfx_res = CheckBufferForLog(m_BuffersUsed, (*it3), bUsed);
            }
            if (MFX_ERR_NONE == mfx_res)
            {
                mfx_res = CheckBufferForLog(m_BuffersToBeSent, (*it3), bUsed);
            }

            if ((MFX_ERR_NONE == mfx_res) && !bUsed)
            {
                bFound = true;
                MfxOmxBufferSwapPair *pSwapPair = (MfxOmxBufferSwapPair*) calloc(1, sizeof(MfxOmxBufferSwapPair));
                if (pSwapPair)
                {
                    pSwapPair->pAddBufInfoFrom = (*it3);
                    pSwapPair->pAddBufInfoTo = (*ppAddBufInfo);
                    m_SwapPool.push_back(pSwapPair);
                    (*ppAddBufInfo) = (*it3);
                    (*ppBuffer)->pOutputPortPrivate = (*it3);
                }
                else
                    mfx_res = MFX_ERR_MEMORY_ALLOC;
                break;
            }
        }
    }

    if (!bFound)
    {
        OMX_BUFFERHEADERTYPE* pBufferUnlocked;
        m_BuffersUnlocked.Get(&pBufferUnlocked, m_pNilBuffer);
        if (pBufferUnlocked)
        {
            (*ppAddBufInfo) = (MfxOmxBufferInfo*)pBufferUnlocked->pOutputPortPrivate;
            pBufferUnlocked->pOutputPortPrivate = (*ppBuffer)->pOutputPortPrivate;
            (*ppBuffer)->pOutputPortPrivate = (*ppAddBufInfo);

            if (!m_BuffersLocked.Add(&pBufferUnlocked))
            {
                mfx_res = MFX_ERR_UNKNOWN;
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBufferUnlocked, mfx_res);
            }
        }
        else
        {
            MFX_OMX_LOG_ERROR("All ANW buffers are locked. Cannot make a swap");
            mfx_res = MFX_ERR_UNKNOWN;
        }
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::SwapBufferInfo(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (m_bOnFlySurfacesAllocation && m_bANWBufferInMetaData)
    {
        MfxOmxBufferInfo* pAddBufInfo = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;

        bool bSwap;
        do {
            bSwap = false;
            for (std::list<MfxOmxBufferSwapPair*>::iterator it1 = m_SwapPool.begin(); it1 != m_SwapPool.end(); it1++)
            {
                if ((*it1) && ((*it1)->pAddBufInfoFrom == pAddBufInfo))
                {
                    pAddBufInfo = (*it1)->pAddBufInfoTo;
                    pBuffer->pOutputPortPrivate = (*it1)->pAddBufInfoTo;
                    MFX_OMX_FREE(*it1);
                    m_SwapPool.erase(it1);
                    bSwap = true;
                    break;
                }
            }
        } while (bSwap);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::UseBuffer(OMX_BUFFERHEADERTYPE* pBuffer, mfxFrameInfo & mfx_info, bool bChangeOutputPortSettings)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    mfxStatus mfx_res = MFX_ERR_NONE;
    // load input buffer for meta data in output buffers mode
    if (m_bOnFlySurfacesAllocation && m_pFrameAllocator && !bChangeOutputPortSettings)
    {
        MfxOmxVaapiFrameAllocator* allocator = (MfxOmxVaapiFrameAllocator*)m_pFrameAllocator;

        MetadataBuffer buffer;
        MFX_OMX_ZERO_MEMORY(buffer);
        mfx_res = mfx_omx_get_metadatabuffer_info(pBuffer->pBuffer + pBuffer->nOffset, pBuffer->nAllocLen, &buffer);

        MfxOmxBufferInfo* pAddBufInfo = (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate;

        if (MFX_ERR_NONE == mfx_res)
        {
            if (MfxMetadataBufferTypeANWBuffer == buffer.type)
            {
                if (buffer.pFenceFd)
                {
                    MFX_OMX_LOG_INFO_IF(g_OmxLogLevel, "ANWbuffer %p, gralloc handle %p, fence %d", buffer.anw_buffer, buffer.handle, *(buffer.pFenceFd));
                    if (*(buffer.pFenceFd) >= 0)
                    {
                        android::sp<android::Fence> fence = new android::Fence(*(buffer.pFenceFd));
                        *(buffer.pFenceFd) = -1;

                        if (android::OK != fence->wait(1000))
                        {
                            MFX_OMX_LOG_ERROR("Timed out waiting on input fence");
                            mfx_res = MFX_ERR_UNKNOWN;
                        }
                    }
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    bool bSwap;
                    do {
                        bSwap = false;
                        for (std::list<MfxOmxBufferSwapPair*>::iterator it1 = m_SwapPool.begin(); it1 != m_SwapPool.end(); it1++)
                        {
                            if ((*it1) && ((*it1)->pAddBufInfoFrom == pAddBufInfo))
                            {
                                pAddBufInfo = (*it1)->pAddBufInfoTo;
                                pBuffer->pOutputPortPrivate = (*it1)->pAddBufInfoTo;
                                MFX_OMX_FREE(*it1);
                                m_SwapPool.erase(it1);
                                bSwap = true;
                                break;
                            }
                        }
                    } while (bSwap);
                    if (pAddBufInfo)
                    {
                        bool bFound = false;
                        for (std::list<MfxOmxBufferInfo*>::iterator it2 = m_BufferInfoPool.begin(); it2 != m_BufferInfoPool.end(); it2++)
                        {
                            if ((*it2) == pAddBufInfo)
                            {
                                bFound = true;
                                break;
                            }
                        }
                        if (!bFound) m_BufferInfoPool.push_back(pAddBufInfo);
                    }
                    if (pAddBufInfo &&
                        pAddBufInfo->sSurface.Data.MemId &&
                        ((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface &&
                        (*((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface != VA_INVALID_ID))
                    {
                        if ((const mfxU8*)buffer.handle != ((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_key)
                        {
                            if (pAddBufInfo->sSurface.Data.Locked)
                            {
                                mfx_res = FindAvailableSurface(&pBuffer, &pAddBufInfo);
                            }
                            if (MFX_ERR_NONE == mfx_res)
                            {
                                if (!pAddBufInfo->pAnwBuffer) mfx_res = MFX_ERR_NULL_PTR;
                            }
                            if (MFX_ERR_NONE == mfx_res)
                            {
                                pAddBufInfo->pAnwBuffer->decStrong(NULL);
                                pAddBufInfo->pAnwBuffer = NULL;
                                allocator->FreeExtMID(pAddBufInfo->sSurface.Data.MemId);
                                *((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface = VA_INVALID_ID;
                            }
                        }
                    }
                }
                if (MFX_ERR_NONE == mfx_res)
                {
                    if (pAddBufInfo &&
                        (pAddBufInfo->sSurface.Data.MemId == NULL ||
                        (pAddBufInfo->sSurface.Data.MemId != NULL &&
                        ((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface &&
                        *((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface == VA_INVALID_ID)) &&
                        buffer.anw_buffer)
                    {
                        pAddBufInfo->pAnwBuffer = buffer.anw_buffer;
                        pAddBufInfo->pAnwBuffer->incStrong(NULL);
                        mfx_res = allocator->LoadSurface(buffer.handle, true, mfx_info, &(pAddBufInfo->sSurface.Data.MemId));
                    }
                }
            }
            else
            {
                android::VideoDecoderOutputMetaData *metaData = NULL;

                if (pBuffer->nAllocLen >= sizeof(android::VideoDecoderOutputMetaData))
                    metaData = (android::VideoDecoderOutputMetaData *)(pBuffer->pBuffer);
                else
                    mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;

                if (NULL == metaData) mfx_res = MFX_ERR_NULL_PTR;
                if (MFX_ERR_NONE == mfx_res)
                {
                    if (pAddBufInfo &&
                        (pAddBufInfo->sSurface.Data.MemId == NULL ||
                        (pAddBufInfo->sSurface.Data.MemId != NULL &&
                        ((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface &&
                        *((vaapiMemId*)pAddBufInfo->sSurface.Data.MemId)->m_pSurface == VA_INVALID_ID)))
                    {
                        mfx_res = allocator->LoadSurface(metaData->pHandle, true,
                                                         mfx_info,
                                                         &(pAddBufInfo->sSurface.Data.MemId));
                    }
                }
            }
        }

        if (MFX_ERR_NONE == mfx_res && pAddBufInfo && pAddBufInfo->sSurface.Data.MemId)
        {
            mfx_res = PrepareSurface(pBuffer, &(mfx_info), pAddBufInfo->sSurface.Data.MemId);
        }
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::UseBuffer(pBuffer);
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    MfxOmxBufferInfo* pBufInfo = MfxOmxGetOutputBufferInfo<OMX_BUFFERHEADERTYPE>(pBuffer);

    bIsLocked = false;
    if (pBufInfo)
    {
        if (pBufInfo->sSurface.Data.Locked) bIsLocked = true;
    }
    else mfx_res = MFX_ERR_NULL_PTR;

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

OMX_VIDEO_ERROR_BUFFER* MfxOmxSurfacesPool::GetCurrentErrorItem(MfxOmxBufferInfo* pAddBufInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MfxOmxVideoErrorBuffer* pCurrentErrorBuffer = NULL;
    if ( pAddBufInfo == NULL ) return NULL;

    for (std::list<MfxOmxVideoErrorBuffer *>::iterator it = m_errorList.begin(); it != m_errorList.end(); ++it)
    {
        if ((*it)->index == pAddBufInfo->nBufferIndex && (*it)->errorNumber == 0)
        {
            pCurrentErrorBuffer = (*it);
            break;
        }
    }

    if (!pCurrentErrorBuffer)
    {
        if (m_errorList.size() < m_maxErrorCount)
        {
            pCurrentErrorBuffer = (MfxOmxVideoErrorBuffer*) calloc(1, sizeof(MfxOmxVideoErrorBuffer));
            if (pCurrentErrorBuffer)
            {
                pCurrentErrorBuffer->index = pAddBufInfo->nBufferIndex;
                pCurrentErrorBuffer->errorNumber = 0;
                m_errorList.push_back(pCurrentErrorBuffer);
                MFX_OMX_AUTO_TRACE_MSG("Created new MfxOmxVideoErrorBuffer item");
            }
        }
        else
        {
            pCurrentErrorBuffer = m_errorList.front();
            m_errorList.pop_front();
            m_errorList.push_back(pCurrentErrorBuffer);
            pCurrentErrorBuffer->index = pAddBufInfo->nBufferIndex;
            pCurrentErrorBuffer->errorNumber = 0;

        }
    }

    return (OMX_VIDEO_ERROR_BUFFER*) pCurrentErrorBuffer;
}

/*------------------------------------------------------------------------------*/
mfxStatus MfxOmxSurfacesPool::SetCommonErrors(MfxOmxBufferInfo* pAddBufInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxFrameData* frameData = &(pAddBufInfo->sSurface.Data);

    OMX_VIDEO_ERROR_BUFFER* pCurrentErrorBuffer = GetCurrentErrorItem(pAddBufInfo);
    if (!pCurrentErrorBuffer) return MFX_ERR_NULL_PTR;

    if (frameData->Corrupted & MFX_CORRUPTION_MINOR ||
        frameData->Corrupted & MFX_CORRUPTION_MAJOR)
        if (pCurrentErrorBuffer->errorNumber + 1 < MAX_ERR_NUM)
            pCurrentErrorBuffer->errorArray[++pCurrentErrorBuffer->errorNumber].type = OMX_Decode_MBError;

    if (frameData->Corrupted & MFX_CORRUPTION_REFERENCE_FRAME ||
        frameData->Corrupted & MFX_CORRUPTION_REFERENCE_LIST ||
        frameData->Corrupted & MFX_CORRUPTION_ABSENT_TOP_FIELD ||
        frameData->Corrupted & MFX_CORRUPTION_ABSENT_BOTTOM_FIELD)
        if (pCurrentErrorBuffer->errorNumber < MAX_ERR_NUM-1)
            pCurrentErrorBuffer->errorArray[++pCurrentErrorBuffer->errorNumber].type = OMX_Decode_RefMissing;

    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::SetHeaderErrors(mfxFrameSurface1 *pSurfaces, mfxU32 isHeaderCorrupted)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    OMX_BUFFERHEADERTYPE* pBuffer = GetBufferByCustomBuffer(pSurfaces);
    if ( pBuffer == NULL )
        return MFX_ERR_NULL_PTR;

    MfxOmxBufferInfo* pAddBufInfo = MfxOmxGetOutputBufferInfo(pBuffer);
    if ( pAddBufInfo == NULL )
        return MFX_ERR_NULL_PTR;

    OMX_VIDEO_ERROR_BUFFER* pCurrentErrorBuffer = GetCurrentErrorItem(pAddBufInfo);
    if (!pCurrentErrorBuffer)
        return MFX_ERR_NULL_PTR;

    if (isHeaderCorrupted)
        if (pCurrentErrorBuffer->errorNumber + 1 < MAX_ERR_NUM)
            pCurrentErrorBuffer->errorArray[++pCurrentErrorBuffer->errorNumber].type = OMX_Decode_HeaderError;

    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxSurfacesPool::FillOutputErrorBuffer(OMX_VIDEO_ERROR_BUFFER* pErrorBuffer, OMX_U32 nErrorBufIndex)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (!pErrorBuffer)
        return MFX_ERR_NULL_PTR;

    memset(pErrorBuffer, 0, sizeof(OMX_VIDEO_ERROR_BUFFER));
    mfxU32 index = 0;

    for (std::list<MfxOmxVideoErrorBuffer *>::iterator it = m_errorList.begin(); it != m_errorList.end(); ++it)
    {
        if ((*it)->index == nErrorBufIndex)
        {
            *pErrorBuffer = *(static_cast<OMX_VIDEO_ERROR_BUFFER*>(*it));
            memset((*it), 0, sizeof(OMX_VIDEO_ERROR_BUFFER));
            break;
        }

        if (index >= m_maxErrorCount)
            break;
    }

    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

void MfxOmxSurfacesPool::DisplaySurface(bool bErrorReportingEnabled, bool isReinit)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    OMX_BUFFERHEADERTYPE* pBuffer = DequeueOutputBufferForSending();
    MfxOmxBufferInfo* pAddBufInfo = MfxOmxGetOutputBufferInfo(pBuffer);
    bool isGoodForDisplay = true;

    if (pBuffer && pAddBufInfo)
    {
        // synchronizing parameters
        if (!(pAddBufInfo->pSyncPoint) && !isReinit)
        { // that's special frame indicating EOS
            pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
        }
        else
        {
            pBuffer->nTimeStamp = MFX2OMX_TIME(pAddBufInfo->sSurface.Data.TimeStamp);
            pBuffer->nFilledLen = pBuffer->nAllocLen;
            pBuffer->nFlags = 0;

            /* There are streams which contain slices with duplicated POC.
               In that case MSDK can return decoded surfaces in wrong order because
               MSDK doesn't parse PTS, just copy from input to output. To keep output
               pts increasing we copy pts from prev frame if it had a higher value. Generally it
               happens with change P->B slice and duplicated sequence has length no more
               than 2 slices. */
            /* There are stream which roll over the timestamp after some time.
               To make sure that is not a stream with duplicated POC the difference of current and prev timestamps
               should be higher than 90kHz(90000)*/

            if(    m_latestTS < pBuffer->nTimeStamp
               || (m_latestTS > pBuffer->nTimeStamp && (m_latestTS - pBuffer->nTimeStamp) > MFX_OMX_TIME_STAMP_FREQ))
            {
                m_latestTS = pBuffer->nTimeStamp;
            }
            else
            {
                pBuffer->nTimeStamp = m_latestTS;
            }

            if (pAddBufInfo->sSurface.Data.Corrupted)
            {
                if (bErrorReportingEnabled)
                {
                    pBuffer->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;
                    SetCommonErrors(pAddBufInfo);
                }
                else if (MFX_CODEC_VC1 == m_codecId)
                {
                    /* It's right behavior to set flag OMX_BUFFERFLAG_DATACORRUPT,
                    but stagefright doesn't process this flag.
                    So corrupted frames causes problem with trumbnail generation
                    so we skip corrupted frame and send it to decoder again. */
                    isGoodForDisplay = false;
                }
            }

#if defined(OMX_BUFFERFLAG_TFF) && defined(OMX_BUFFERFLAG_BFF)
            if (MFX_PICSTRUCT_FIELD_TFF & pAddBufInfo->sSurface.Info.PicStruct)
                pBuffer->nFlags |= OMX_BUFFERFLAG_TFF;
            else if (MFX_PICSTRUCT_FIELD_BFF & pAddBufInfo->sSurface.Info.PicStruct)
                pBuffer->nFlags |= OMX_BUFFERFLAG_BFF;
#endif
            if (m_dbg_file)
            {
                if (pAddBufInfo->sSurface.Data.MemId)
                { // hw frame
                    if (m_pFrameAllocator)
                    {
                        mfxFrameData frame_data;
                        MFX_OMX_ZERO_MEMORY(frame_data);

                        if (MFX_ERR_NONE == m_pFrameAllocator->Lock(m_pFrameAllocator->pthis,
                                                                    pAddBufInfo->sSurface.Data.MemId,
                                                                    &frame_data))
                        {
                            switch (pAddBufInfo->sSurface.Info.FourCC)
                            {
                                case MFX_FOURCC_NV12:
                                    mfx_omx_dump_YUV_from_NV12_data(m_dbg_file,
                                                                    &frame_data,
                                                                    &(pAddBufInfo->sSurface.Info),
                                                                    frame_data.Pitch);
                                    break;
                                case MFX_FOURCC_P010:  // FIXME
                                    mfx_omx_dump_YUV_from_P010_data(m_dbg_file,
                                                                    &frame_data,
                                                                    &(pAddBufInfo->sSurface.Info));
                                    break;
                            };
                        }
                        m_pFrameAllocator->Unlock(m_pFrameAllocator->pthis,
                                                  pAddBufInfo->sSurface.Data.MemId,
                                                  &frame_data);
                    }
                }
                else
                { // sw frame
                    mfx_omx_dump_YUV_from_NV12_data(m_dbg_file,
                                                    &(pAddBufInfo->sSurface.Data),
                                                    &(pAddBufInfo->sSurface.Info),
                                                    m_nPitch);
                }
            }

            if (bErrorReportingEnabled)
            {
                MfxOmxVideoErrorBuffer* pCurrentErrorBuffer = (MfxOmxVideoErrorBuffer*) GetCurrentErrorItem(pAddBufInfo);
                if (!pCurrentErrorBuffer) return;

                if (pCurrentErrorBuffer->errorNumber)
                    pBuffer->pPlatformPrivate = (void *)(intptr_t)pCurrentErrorBuffer->index;
                else
                    pBuffer->pPlatformPrivate = (void *)(intptr_t)0xffffffff; // means no error for this buffer
            }
        }

        MFX_OMX_AUTO_TRACE_I64(pBuffer->nTimeStamp);
        MFX_OMX_AUTO_TRACE_U32(pBuffer->nFlags);

        if (isGoodForDisplay)
        {
            MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
        }
        else
            MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::UseBuffer(pBuffer); // re-send to decoder surface queue
    }
}

/*------------------------------------------------------------------------------*/

mfxU16 MfxOmxSurfacesPool::GetPitch(mfxFrameInfo* pFrameInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxU16 nPitch = 0;

    if (pFrameInfo) nPitch = pFrameInfo->Width;
    else nPitch = m_nPitch;
    MFX_OMX_AUTO_TRACE_I32(nPitch);
    return nPitch;
}

void MfxOmxSurfacesPool::SetMaxErrorCount(mfxU32 maxErrors){
    m_maxErrorCount = maxErrors;
}

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

#ifndef __MFX_OMX_BUFFERS_H__
#define __MFX_OMX_BUFFERS_H__

#include "mfx_omx_utils.h"

/*------------------------------------------------------------------------------*/

/** Framework specific buffer AddRef() function implementation. */
template <typename T>
inline void MfxOmxBufferAddRef(T* pBuffer);

/** Framework specific buffer Release() function implementation. */
template <typename T>
inline void MfxOmxBufferRelease(T* pBuffer);

/**
 * Framework specific function which returns MfxOmxBufferInfo attribute
 * pointer from the given input buffer.
 */
template <typename T>
inline MfxOmxBufferInfo* MfxOmxGetInputBufferInfo(T* pBuffer);

/**
 * Framework specific function which returns MfxOmxBufferInfo attribute
 * pointer from the given output buffer.
 */
template <typename T>
inline MfxOmxBufferInfo* MfxOmxGetOutputBufferInfo(T* pBuffer);

/** Gets Media SDK input custom buffer from framework specific buffer. */
template <typename T, typename B>
inline B* MfxOmxGetInputCustomBuffer(T* pBuffer);

/** Gets Media SDK output custom buffer from framework specific buffer. */
template <typename T, typename B>
inline B* MfxOmxGetOutputCustomBuffer(T* pBuffer);

/*------------------------------------------------------------------------------*/

template<>
inline void MfxOmxBufferAddRef<OMX_BUFFERHEADERTYPE>(
    OMX_BUFFERHEADERTYPE* /*pBuffer*/)
{}

template <>
inline MfxOmxBufferInfo* MfxOmxGetInputBufferInfo(OMX_BUFFERHEADERTYPE* pBuffer)
{
    return (pBuffer)? (MfxOmxBufferInfo*)pBuffer->pInputPortPrivate : NULL;
}

template <>
inline MfxOmxBufferInfo* MfxOmxGetOutputBufferInfo(OMX_BUFFERHEADERTYPE* pBuffer)
{
    return (pBuffer)? (MfxOmxBufferInfo*)pBuffer->pOutputPortPrivate : NULL;
}

template <>
inline mfxFrameSurface1* MfxOmxGetInputCustomBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MfxOmxBufferInfo* pBufInfo = MfxOmxGetInputBufferInfo<OMX_BUFFERHEADERTYPE>(pBuffer);

    if (pBufInfo) return &(pBufInfo->sSurface);
    return NULL;
}

template <>
inline mfxFrameSurface1* MfxOmxGetOutputCustomBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MfxOmxBufferInfo* pBufInfo = MfxOmxGetOutputBufferInfo<OMX_BUFFERHEADERTYPE>(pBuffer);

    if (pBufInfo) return &(pBufInfo->sSurface);
    return NULL;
}

template <>
inline mfxBitstream* MfxOmxGetOutputCustomBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MfxOmxBufferInfo* pBufInfo = MfxOmxGetOutputBufferInfo<OMX_BUFFERHEADERTYPE>(pBuffer);

    if (pBufInfo) return &(pBufInfo->sBitstream);
    return NULL;
}

/*------------------------------------------------------------------------------*/

typedef void* MfxOmxPoolId;

template <typename T>
class MfxOmxBuffersCallback
{
public:
    virtual ~MfxOmxBuffersCallback(void){}

    /** Signals that buffer was released. */
    virtual void BufferReleased(MfxOmxPoolId id, T* pBuffer, mfxStatus error) = 0;
};

/*------------------------------------------------------------------------------*/

#ifndef MFX_OMX_RELEASE_BUFFER
    #define MFX_OMX_RELEASE_BUFFER(_callback, _id, _buffer, _error) \
    { \
        if (_buffer) \
        { \
            this->ReleaseBuffer(_buffer); \
            if (_callback) \
            { \
                _callback->BufferReleased(_id, _buffer, _error); \
            } \
            _buffer = NULL; \
        } \
    }
#endif

/*------------------------------------------------------------------------------*/

template <typename T>
class MfxOmxInputBuffersPool
{
public:
    MfxOmxInputBuffersPool(mfxStatus &error):
        m_pBuffersCallback(NULL),
        m_pCurrentBufferToProcess(NULL),
        m_pNilBuffer(NULL)
    { error = MFX_ERR_NONE; }
    virtual ~MfxOmxInputBuffersPool(void){}

    /** Resets pool releasing all buffers. */
    virtual mfxStatus Reset(void);
    /** Sets callback to call on buffer release. */
    virtual mfxStatus SetBuffersCallback(MfxOmxBuffersCallback<T>* pCallback);
    /** Adds new income buffer to the pool. */
    virtual mfxStatus UseBuffer(T* pBuffer);
    /** Gets next buffer from the pool. */
    virtual T* GetBuffer(void);
    /** Gets current buffer */
    virtual T* GetCurrentBuffer(void);
    /** Release buffer corresponding data */
    virtual mfxStatus ReleaseBuffer(T* pBuffer);
    /**
     * Function determines whether given buffer is locked or not.
     * @note Since buffer can be of the framework format and locked/free criteria
     * may use some buffer properties, erros may occur (property absent). So, this
     * function returns an error instead of bool variable stating locked state of
     * a buffer.
     */
    virtual mfxStatus IsBufferLocked(T* pBuffer, bool& bIsLocked) = 0;

    /** Returns pool id. */
    inline MfxOmxPoolId GetPoolId(void) { return (MfxOmxPoolId)this; }

protected: // variables
    /** Callback thru which pool will inform that buffer was released. */
    MfxOmxBuffersCallback<T>* m_pBuffersCallback;

    /** List of incoming buffers. */
    MfxOmxRing<T*> m_BuffersToProcess;

    /** Buffer taken from m_BuffersToProcess list but not processed yet. */
    T* m_pCurrentBufferToProcess;

    /** Synchronization mutex. */
    MfxOmxMutex m_mutex;

    /** Nil buffer: buffer marking that current buffer is NULL. */
    T* m_pNilBuffer;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxInputBuffersPool)
};

/*------------------------------------------------------------------------------*/

template <typename T>
mfxStatus MfxOmxInputBuffersPool<T>::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    T* pBuffer = NULL;

    while (m_BuffersToProcess.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    if (m_pCurrentBufferToProcess)
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), m_pCurrentBufferToProcess, MFX_ERR_NONE);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T>
mfxStatus MfxOmxInputBuffersPool<T>::SetBuffersCallback(MfxOmxBuffersCallback<T>* pCallback)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!pCallback) mfx_res = MFX_ERR_NULL_PTR;
    if ((MFX_ERR_NONE == mfx_res) && m_pBuffersCallback) mfx_res = MFX_ERR_UNDEFINED_BEHAVIOR;
    if (MFX_ERR_NONE == mfx_res)
    {
        m_pBuffersCallback = pCallback;
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T>
mfxStatus MfxOmxInputBuffersPool<T>::UseBuffer(T* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    {
        MfxOmxBufferAddRef<T>(pBuffer);
        if (!m_pCurrentBufferToProcess && 0 == m_BuffersToProcess.GetItemsCount())
            m_pCurrentBufferToProcess = pBuffer;
        else if (!m_BuffersToProcess.Add(&pBuffer))
        {
            mfx_res = MFX_ERR_UNKNOWN;
            MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, mfx_res);
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T>
T* MfxOmxInputBuffersPool<T>::GetBuffer(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    T* pBuffer = NULL;

    if (!m_pCurrentBufferToProcess)
    {
        MFX_OMX_AUTO_TRACE_I32(m_BuffersToProcess.GetItemsCount());
        m_BuffersToProcess.Get(&pBuffer, m_pNilBuffer);
    }
    else
    {
        pBuffer = m_pCurrentBufferToProcess;
        m_pCurrentBufferToProcess = NULL;
    }
    MFX_OMX_AUTO_TRACE_P(pBuffer);
    return pBuffer;
}

/*------------------------------------------------------------------------------*/

template <typename T>
T* MfxOmxInputBuffersPool<T>::GetCurrentBuffer(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    T* pBuffer = NULL;

    pBuffer = m_pCurrentBufferToProcess;

    MFX_OMX_AUTO_TRACE_P(pBuffer);
    return pBuffer;
}

/*------------------------------------------------------------------------------*/

template <typename T>
mfxStatus MfxOmxInputBuffersPool<T>::ReleaseBuffer(T* pBuffer)
{
    MFX_OMX_UNUSED(pBuffer);
    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
class MfxOmxInputRefBuffersPool : public MfxOmxInputBuffersPool<T>
{
public:
    MfxOmxInputRefBuffersPool(mfxStatus &error):
        MfxOmxInputBuffersPool<T>(error)
    {}
    virtual ~MfxOmxInputRefBuffersPool(void){}

    /** Resets pool releasing all buffers. */
    virtual mfxStatus Reset(void);
    /** Adds new income buffer to the pool. */
    virtual mfxStatus UseBuffer(T* pBuffer);
    /** Gets next buffer from the pool in custom format. */
    virtual B* GetCustomBuffer(void);
    /** Releases associated buffer config. */
    virtual MfxOmxInputConfig* GetInputConfig(void);
    /** Keeps buffer inside pool till it will not be unlocked. */
    virtual mfxStatus KeepBuffer(B* pCustomBuffer);
    /**
     * Checks whether any buffer in m_BuffersUsed array was unlocked and releases
     * it if found.
     */
    virtual mfxStatus CheckBuffers(void);

protected: // functions
    /**
     * Version of CheckBuffer function which can be invoked from other functions
     * of this class.
     */
    virtual mfxStatus CheckBuffers_internal(void);

protected: // variables
    /** List of buffers currently in use which can't be yet released. */
    MfxOmxRing<T*> m_BuffersUsed;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxInputRefBuffersPool)
};

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxInputRefBuffersPool<T,B>::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    T* pBuffer = NULL;

    while (m_BuffersUsed.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(this->m_pBuffersCallback, this->GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    mfx_res = MfxOmxInputBuffersPool<T>::Reset();
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxInputRefBuffersPool<T,B>::UseBuffer(T* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = MfxOmxInputBuffersPool<T>::UseBuffer(pBuffer);
    if (MFX_ERR_NONE == mfx_res) mfx_res = CheckBuffers();

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
B* MfxOmxInputRefBuffersPool<T,B>::GetCustomBuffer(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(this->m_mutex);
    B* pCustomBuffer = NULL;

    if (!this->m_pCurrentBufferToProcess)
    {
        MFX_OMX_AUTO_TRACE_I32(this->m_BuffersToProcess.GetItemsCount());
        this->m_BuffersToProcess.Get(&(this->m_pCurrentBufferToProcess), this->m_pNilBuffer);
    }
    if (this->m_pCurrentBufferToProcess)
    {
        pCustomBuffer = MfxOmxGetInputCustomBuffer<T,B>(this->m_pCurrentBufferToProcess);
    }
    MFX_OMX_AUTO_TRACE_P(pCustomBuffer);
    return pCustomBuffer;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
MfxOmxInputConfig* MfxOmxInputRefBuffersPool<T,B>::GetInputConfig(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(this->m_mutex);
    MfxOmxInputConfig* config = NULL;

    if (this->m_pCurrentBufferToProcess)
    {
        MfxOmxBufferInfo* pBufInfo = MfxOmxGetInputBufferInfo<OMX_BUFFERHEADERTYPE>(this->m_pCurrentBufferToProcess);

        config = &(pBufInfo->config);
        MFX_OMX_AUTO_TRACE_P(config->control);
        MFX_OMX_AUTO_TRACE_P(config->mfxparams);
    }
    return config;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxInputRefBuffersPool<T,B>::KeepBuffer(B* pCustomBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(this->m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    T* pBuffer = NULL;
    B* pCustomBufferToCheck = NULL;
    bool bIsLocked = false;

    if (this->m_pCurrentBufferToProcess)
    {
        pCustomBufferToCheck = MfxOmxGetInputCustomBuffer<T,B>(this->m_pCurrentBufferToProcess);
        /** @todo It may be needed in the future to search for pCustomBuffer in m_BuffersToProcess. */
        if (pCustomBufferToCheck == pCustomBuffer)
        {
            pBuffer = this->m_pCurrentBufferToProcess;
            this->m_pCurrentBufferToProcess = NULL;

            // setting new current buffer
            this->m_BuffersToProcess.Get(&(this->m_pCurrentBufferToProcess), this->m_pNilBuffer);
        }
        else mfx_res = MFX_ERR_UNDEFINED_BEHAVIOR;
    }
    else mfx_res = MFX_ERR_UNDEFINED_BEHAVIOR;

    if (MFX_ERR_NONE == mfx_res) mfx_res = CheckBuffers_internal();
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = this->IsBufferLocked(pBuffer, bIsLocked);
        if (MFX_ERR_NONE == mfx_res)
        {
            if (bIsLocked)
            {
                MfxOmxBufferAddRef<T>(pBuffer);
                if (!m_BuffersUsed.Add(&pBuffer))
                {
                    mfx_res = MFX_ERR_UNKNOWN;
                    MFX_OMX_RELEASE_BUFFER(this->m_pBuffersCallback, this->GetPoolId(), pBuffer, mfx_res);
                }
            }
            else
            {
                MFX_OMX_RELEASE_BUFFER(this->m_pBuffersCallback, this->GetPoolId(), pBuffer, MFX_ERR_NONE);
            }
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxInputRefBuffersPool<T,B>::CheckBuffers(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(this->m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;

    mfx_res = CheckBuffers_internal();
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxInputRefBuffersPool<T,B>::CheckBuffers_internal(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE, mfx_sts = MFX_ERR_NONE;
    T* pBuffer = NULL;
    bool bIsLocked = false;
    mfxU32 i = 0, buffers_num = 0;

    buffers_num = m_BuffersUsed.GetItemsCount();
    for (i = 0; i < buffers_num; ++i)
    {
        if (m_BuffersUsed.Get(&pBuffer))
        {
            mfx_sts = this->IsBufferLocked(pBuffer, bIsLocked);
            if (MFX_ERR_NONE == mfx_sts)
            {
                if (bIsLocked)
                { // returning buffer back if it is still locked
                    if (!m_BuffersUsed.Add(&pBuffer))
                    {
                        mfx_sts = MFX_ERR_UNKNOWN;
                        MFX_OMX_RELEASE_BUFFER(this->m_pBuffersCallback, this->GetPoolId(), pBuffer, mfx_res);
                    }
                }
                else
                { // releasing buffer if it is unlocked
                    MFX_OMX_RELEASE_BUFFER(this->m_pBuffersCallback, this->GetPoolId(), pBuffer, MFX_ERR_NONE);
                }
            }
            if ((MFX_ERR_NONE == mfx_res) && (MFX_ERR_NONE != mfx_sts)) mfx_res = mfx_sts;
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
class MfxOmxOutputBuffersPool
{
public:
    MfxOmxOutputBuffersPool(mfxStatus &error):
        m_pBuffersCallback(NULL),
        m_pCurrentBufferUnlocked(NULL),
        m_pCurrentBufferToBeSent(NULL),
        m_pNilBuffer(NULL)
    { error = MFX_ERR_NONE; }
    virtual ~MfxOmxOutputBuffersPool(void){}

    /** Resets pool releasing all buffers. */
    virtual mfxStatus Reset(void);
    /** Sets callback to call on buffer release. */
    virtual mfxStatus SetBuffersCallback(MfxOmxBuffersCallback<T>* pCallback);
    /**
     * Adds new buffer to the pool. Function will place the buffer in either
     * m_BuffersUnlocked or m_BuffersLocked array.
     */
    virtual mfxStatus UseBuffer(T* pBuffer);
    /** Gets unlocked buffer from the pool in custom format. */
    virtual B* GetBuffer(void);
    /** Release buffer corresponding data */
    virtual mfxStatus ReleaseBuffer(T* pBuffer);
    /** Moves given buffer to the m_BuffersToBeSent array*/
    virtual mfxStatus QueueBufferForSending(B* pBuffer, mfxSyncPoint* pSyncPoint);
    /** Gets sync point for the m_pCurrentBufferToBeSent. */
    virtual mfxSyncPoint* GetSyncPoint(void);
    /** Returns current buffer*/
    virtual T* GetOutputBuffer(void) const;
    /** Dequeue buffer ready to be sent to downstream plug-in. */
    virtual T* DequeueOutputBufferForSending(void);

    /**
     * Function determines whether given buffer is locked or free. This function
     * is a helper one which gives a criteria to place the buffer either in
     * m_BuffersUnlocked or m_BuffersLocked array.
     * @note Since buffer can be of the framework format and locked/free criteria
     * may use some buffer properties, erros may occur (property absent). So, this
     * function returns an error instead of bool variable stating locked state of
     * a buffer.
     */
    virtual mfxStatus IsBufferLocked(T* pBuffer, bool& bIsLocked) = 0;

    /** Returns pool id. */
    inline MfxOmxPoolId GetPoolId(void) { return (MfxOmxPoolId)this; }

protected: // functions
    /**
     * Redistributes buffers between buffer arrays checking whether any buffer
     * was unlocked.
     */
    virtual mfxStatus SyncBuffers(void);
    /**
     * Searches among m_BuffersUsed (and m_pCurrentBufferUnlocked) item which
     * contains given custom buffer.
     */
    virtual T* GetBufferByCustomBuffer(B* pCustomBuffer);

protected: // variables
    /** Callback thru which pool will inform that buffer was released. */
    MfxOmxBuffersCallback<T>* m_pBuffersCallback;

    /** List of free buffers. */
    MfxOmxRing<T*> m_BuffersUnlocked;
    /** List of locked buffers. */
    MfxOmxRing<T*> m_BuffersLocked;
    /** List of free buffers. */
    MfxOmxRing<T*> m_BuffersUsed;
    /** List of locked buffers. */
    MfxOmxRing<T*> m_BuffersToBeSent;

    /** Buffer taken from m_BuffersUnlocked list but not processed yet.*/
    T* m_pCurrentBufferUnlocked;
    /** Buffer taken from m_BuffersToBeSent list but not processed yet.*/
    T* m_pCurrentBufferToBeSent;

    /** Synchronization mutex. */
    MfxOmxMutex m_mutex;

    /** Nil buffer: buffer marking that current buffer is NULL. */
    T* m_pNilBuffer;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxOutputBuffersPool)
};

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::Reset(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    T* pBuffer = NULL;

    while (m_BuffersUnlocked.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    while (m_BuffersLocked.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    while (m_BuffersUsed.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    while (m_BuffersToBeSent.Get(&pBuffer))
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_NONE);
    }
    if (m_pCurrentBufferUnlocked)
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), m_pCurrentBufferUnlocked, MFX_ERR_NONE);
    }
    if (m_pCurrentBufferToBeSent)
    {
        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), m_pCurrentBufferToBeSent, MFX_ERR_NONE);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::SetBuffersCallback(MfxOmxBuffersCallback<T>* pCallback)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!pCallback) mfx_res = MFX_ERR_NULL_PTR;
    if ((MFX_ERR_NONE == mfx_res) && m_pBuffersCallback) mfx_res = MFX_ERR_UNDEFINED_BEHAVIOR;
    if (MFX_ERR_NONE == mfx_res)
    {
        m_pBuffersCallback = pCallback;
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::SyncBuffers(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    T *pBuffer = NULL;
    mfxU32 i = 0, buffers_num = 0;
    bool bIsLocked = false;

    // searching among buffers returned to the plug-in for the unlocked ones
    buffers_num = m_BuffersLocked.GetItemsCount();
    for (i = 0; i < buffers_num; ++i)
    {
        if (m_BuffersLocked.Get(&pBuffer))
        {
            IsBufferLocked(pBuffer, bIsLocked);
            if (bIsLocked)
            { // returning buffer back to the list if it is still locked by Media SDK
                if (!m_BuffersLocked.Add(&pBuffer))
                {
                    MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_UNKNOWN);
                    if (MFX_ERR_NONE == mfx_res) mfx_res = MFX_ERR_UNKNOWN;
                }
            }
            else
            { // moving buffer to the fully free buffers list if it is unlocked by Media SDK
                if (!m_BuffersUnlocked.Add(&pBuffer))
                {
                    MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_UNKNOWN);
                    if (MFX_ERR_NONE == mfx_res) mfx_res = MFX_ERR_UNKNOWN;
                }
            }
        }
    }
    // searching among used buffers for the unlocked ones
    buffers_num = m_BuffersUsed.GetItemsCount();
    for (i = 0; i < buffers_num; ++i)
    {
        if (m_BuffersUsed.Get(&pBuffer))
        {
            IsBufferLocked(pBuffer, bIsLocked);
            if (bIsLocked)
            { // returning buffer back to the list if it is still locked by Media SDK
                if (!m_BuffersUsed.Add(&pBuffer))
                {
                    MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_UNKNOWN);
                    if (MFX_ERR_NONE == mfx_res) mfx_res = MFX_ERR_UNKNOWN;
                }
            }
            else
            { // moving buffer to the fully free buffers list if it is unlocked by Media SDK
                if (!m_BuffersUnlocked.Add(&pBuffer))
                {
                    MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_UNKNOWN);
                    if (MFX_ERR_NONE == mfx_res) mfx_res = MFX_ERR_UNKNOWN;
                }
            }
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::UseBuffer(T* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    bool bIsLocked = false;

    if (!pBuffer) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res) mfx_res = SyncBuffers();
    if (MFX_ERR_NONE == mfx_res) mfx_res = IsBufferLocked(pBuffer, bIsLocked);
    if (MFX_ERR_NONE == mfx_res)
    {
//        MFX_OMX_AT__OMX_BUFFERHEADERTYPE((*pBuffer));
        MfxOmxBufferAddRef<T>(pBuffer);
        if (bIsLocked)
        {
            if (!m_BuffersLocked.Add(&pBuffer))
            {
                mfx_res = MFX_ERR_UNKNOWN;
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, mfx_res);
            }
        }
        else
        {
            if (!m_pCurrentBufferUnlocked) m_pCurrentBufferUnlocked = pBuffer;
            else if (!m_BuffersUnlocked.Add(&pBuffer))
            {
                mfx_res = MFX_ERR_UNKNOWN;
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, mfx_res);
            }
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
B* MfxOmxOutputBuffersPool<T,B>::GetBuffer(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    bool bIsLocked = false;
    B* pCustomBuffer = NULL;

    SyncBuffers();
    if (m_pCurrentBufferUnlocked)
    {
        IsBufferLocked(m_pCurrentBufferUnlocked, bIsLocked);
        if (bIsLocked)
        { // surface was locked but was not marked to be displayed; we need another surface
            if (!m_BuffersUsed.Add(&m_pCurrentBufferUnlocked))
            {
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), m_pCurrentBufferUnlocked, MFX_ERR_UNKNOWN);
            }
            m_BuffersUnlocked.Get(&m_pCurrentBufferUnlocked, m_pNilBuffer);
        }
    }
    else m_BuffersUnlocked.Get(&m_pCurrentBufferUnlocked, m_pNilBuffer);
    if (m_pCurrentBufferUnlocked)
    {
        pCustomBuffer = MfxOmxGetOutputCustomBuffer<T,B>(m_pCurrentBufferUnlocked);
    }
    MFX_OMX_AUTO_TRACE_P(pCustomBuffer);
    return pCustomBuffer;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
T* MfxOmxOutputBuffersPool<T,B>::GetBufferByCustomBuffer(B* pCustomBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    T *pBuffer = NULL;
    B* pCustomBufferToCheck = NULL;

    if (m_pCurrentBufferUnlocked)
    { // firstly checking current unlocked buffer (it can be locked at this moment already)
        pCustomBufferToCheck = MfxOmxGetOutputCustomBuffer<T,B>(m_pCurrentBufferUnlocked);
        if (pCustomBufferToCheck == pCustomBuffer)
        {
            pBuffer = m_pCurrentBufferUnlocked;
            m_pCurrentBufferUnlocked = NULL;
        }
    }
    if (!pBuffer)
    { // if not found searching thru previously used buffers
        mfxU32 i = 0, buffers_num = m_BuffersUsed.GetItemsCount();

        for (i = 0; i < buffers_num; ++i)
        {
            if (m_BuffersUsed.Get(&pBuffer, m_pNilBuffer))
            {
                pCustomBufferToCheck = MfxOmxGetOutputCustomBuffer<T,B>(pBuffer);
                if (pCustomBufferToCheck == pCustomBuffer) break;
                else
                {
                    if (!m_BuffersUsed.Add(&pBuffer))
                    {
                        MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, MFX_ERR_UNKNOWN);
                    }
                    pBuffer = NULL;
                }
            }
        }
    }
    MFX_OMX_AUTO_TRACE_P(pBuffer); // should never be NULL
    return pBuffer;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::ReleaseBuffer(T* pBuffer)
{
    MFX_OMX_UNUSED(pBuffer);
    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxStatus MfxOmxOutputBuffersPool<T,B>::QueueBufferForSending(B* pCustomBuffer, mfxSyncPoint* pSyncPoint)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxStatus mfx_res = MFX_ERR_NONE;
    T* pBuffer = GetBufferByCustomBuffer(pCustomBuffer);
    MfxOmxBufferInfo* pAddBufInfo = MfxOmxGetOutputBufferInfo(pBuffer);

    if (pBuffer && pAddBufInfo)
    {
        pAddBufInfo->pSyncPoint = pSyncPoint;
        if (m_pCurrentBufferToBeSent)
        {
            if (!m_BuffersToBeSent.Add(&pBuffer))
            {
                mfx_res = MFX_ERR_UNKNOWN;
                MFX_OMX_RELEASE_BUFFER(m_pBuffersCallback, GetPoolId(), pBuffer, mfx_res);
            }
        }
        else
        {
            m_pCurrentBufferToBeSent = pBuffer;
        }
    }
    else mfx_res = MFX_ERR_NOT_FOUND; // should not occur

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
mfxSyncPoint* MfxOmxOutputBuffersPool<T,B>::GetSyncPoint(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    mfxSyncPoint* pSyncPoint = NULL;
    MfxOmxBufferInfo* pAddBufInfo = NULL;

    if (m_pCurrentBufferToBeSent)
    {
        pAddBufInfo = MfxOmxGetOutputBufferInfo(m_pCurrentBufferToBeSent);
        pSyncPoint = pAddBufInfo->pSyncPoint;
    }
    MFX_OMX_AUTO_TRACE_P(pSyncPoint);
    return pSyncPoint;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
T* MfxOmxOutputBuffersPool<T,B>::GetOutputBuffer(void) const
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_P(m_pCurrentBufferToBeSent);
    return m_pCurrentBufferToBeSent;
}

/*------------------------------------------------------------------------------*/

template <typename T, typename B>
T* MfxOmxOutputBuffersPool<T,B>::DequeueOutputBufferForSending(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxOmxAutoLock lock(m_mutex);
    T* pBuffer = NULL;

    if (m_pCurrentBufferToBeSent)
    {
        pBuffer = m_pCurrentBufferToBeSent;
        m_BuffersToBeSent.Get(&m_pCurrentBufferToBeSent, m_pNilBuffer);
    }
    MFX_OMX_AUTO_TRACE_P(pBuffer);
    return pBuffer;
}

#endif // #ifndef __MFX_OMX_BUFFERS_H__

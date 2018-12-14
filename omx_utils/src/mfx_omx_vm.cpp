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

#include "mfx_omx_utils.h"
#include <errno.h>

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_vm"

#define VM_CHECK(r , func) { int sts = ( func );  if (sts) r = sts; }

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/
/*                                 S O                                          */
/*------------------------------------------------------------------------------*/

mfx_omx_so_handle mfx_omx_so_load(const char *file_name)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_S(file_name);
    if (!file_name) return NULL;

    dlerror();
    mfx_omx_so_handle handle = (mfx_omx_so_handle) dlopen(file_name, RTLD_NOW);
    if (!handle) { MFX_OMX_LOG_ERROR("%s", dlerror()); }
    return handle;
}

/*------------------------------------------------------------------------------*/

void* mfx_so_get_addr(mfx_omx_so_handle handle, const char *func_name)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_S(func_name);
    if (!handle) return NULL;

    // on Android dlerror returns 'const char'
    char* error = (char*)dlerror(); // reset dlerror status
    void* addr = dlsym(handle, func_name);

    if (!addr)
    {
        error = (char*)dlerror();
        MFX_OMX_LOG_ERROR("%s", error);
        return NULL;
    }
    return addr;
}

/*------------------------------------------------------------------------------*/

void mfx_omx_so_free(mfx_omx_so_handle handle)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    if (!handle) return;

    dlclose(handle);
}

/*------------------------------------------------------------------------------*/
/*                              M I S C                                         */
/*------------------------------------------------------------------------------*/

mfxU32 mfx_omx_get_cpu_num(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    /* On Android *_CONF will return number of _real_ processors. */
    return sysconf(_SC_NPROCESSORS_ONLN);
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/
/*                              M U T E X E S                                   */
/*------------------------------------------------------------------------------*/

MfxOmxMutex::MfxOmxMutex(void)
{
    int res = pthread_mutex_init(&m_mutex, NULL);
    MFX_OMX_THROW_IF(res, std::bad_alloc());
}

/*------------------------------------------------------------------------------*/

MfxOmxMutex::~MfxOmxMutex(void)
{
    pthread_mutex_destroy(&m_mutex);
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxMutex::Lock(void)
{
    return (pthread_mutex_lock(&m_mutex))? MFX_ERR_UNKNOWN: MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxMutex::Unlock(void)
{
    return (pthread_mutex_unlock(&m_mutex))? MFX_ERR_UNKNOWN: MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

bool MfxOmxMutex::Try(void)
{
    return (pthread_mutex_trylock(&m_mutex))? false: true;
}

/*------------------------------------------------------------------------------*/

MfxOmxAutoLock::MfxOmxAutoLock(MfxOmxMutex& mutex):
    m_rMutex(mutex),
    m_bLocked(false)
{
    MFX_OMX_THROW_IF((MFX_ERR_NONE != Lock()), std::bad_alloc());
}

/*------------------------------------------------------------------------------*/

MfxOmxAutoLock::~MfxOmxAutoLock(void)
{
    Unlock();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAutoLock::Lock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (!m_bLocked)
    {
        if (!m_rMutex.Try())
        {
            // add time measurement here to estimate how long you sleep on mutex
            sts = m_rMutex.Lock();
        }
        m_bLocked = true;
    }
    return sts;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxAutoLock::Unlock(void)
{
    mfxStatus sts = MFX_ERR_NONE;
    if (m_bLocked)
    {
        sts = m_rMutex.Unlock();
        m_bLocked = false;
    }
    return sts;
}

/*------------------------------------------------------------------------------*/
/*                              E V E N T S                                     */
/*------------------------------------------------------------------------------*/

MfxOmxEvent::MfxOmxEvent(bool manual, bool state)
{
    m_manual = manual;
    m_state = state;

    int res = pthread_cond_init(&m_event, NULL);
    if (!res)
    {
        res = pthread_mutex_init(&m_mutex, NULL);
        if (res)
        {
            pthread_cond_destroy(&m_event);
        }
    }
    MFX_OMX_THROW_IF(res, std::bad_alloc());
}

/*------------------------------------------------------------------------------*/

MfxOmxEvent::~MfxOmxEvent(void)
{
    pthread_cond_destroy(&m_event);
    pthread_mutex_destroy(&m_mutex);
}

/*------------------------------------------------------------------------------*/

int MfxOmxEvent::Signal(void)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if (!m_state)
        {
            m_state = true;
            if (m_manual) VM_CHECK(res, pthread_cond_broadcast(&m_event))
            else VM_CHECK(res, pthread_cond_signal(&m_event))
        }
        VM_CHECK(res, pthread_mutex_unlock(&m_mutex));
    }
    return res;
}

/*------------------------------------------------------------------------------*/

int MfxOmxEvent::Reset(void)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if (m_state) m_state = false;
        res = pthread_mutex_unlock(&m_mutex);
    }
    return res;
}

/*------------------------------------------------------------------------------*/

int MfxOmxEvent::Wait(void)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if (!m_state) VM_CHECK(res, pthread_cond_wait(&m_event, &m_mutex));
        if (!m_manual) m_state = false;

        VM_CHECK(res, pthread_mutex_unlock(&m_mutex));
    }
    return res;
}

/*------------------------------------------------------------------------------*/

int MfxOmxEvent::TimedWait(mfxU32 msec)
{
    int res = 0;
    if (m_state)
    {
        res = pthread_mutex_lock(&m_mutex);
        if (!res)
        {
            if (!m_state)
            {
                struct timeval tval;
                struct timespec tspec;

                gettimeofday(&tval, NULL);
                msec = 1000 * msec + tval.tv_usec;
                tspec.tv_sec = tval.tv_sec + msec / 1000000;
                tspec.tv_nsec = (msec % 1000000) * 1000;
                VM_CHECK(res, pthread_cond_timedwait(&m_event, &m_mutex, &tspec));
            }

            if (!m_manual) m_state = false;

            VM_CHECK(res, pthread_mutex_unlock(&m_mutex));
        }
    }
    return res;
}

/*------------------------------------------------------------------------------*/
/*                              S E M A P H O R S                               */
/*------------------------------------------------------------------------------*/

MfxOmxSemaphore::MfxOmxSemaphore(mfxU32 count)
{
    m_count = count;

    int res = pthread_cond_init(&m_semaphore, NULL);
    if (!res)
    {
        res = pthread_mutex_init(&m_mutex, NULL);
        if (res)
        {
            pthread_cond_destroy(&m_semaphore);
        }
    }
    MFX_OMX_THROW_IF(res, std::bad_alloc());
}

/*------------------------------------------------------------------------------*/

MfxOmxSemaphore::~MfxOmxSemaphore(void)
{
    pthread_cond_destroy(&m_semaphore);
    pthread_mutex_destroy(&m_mutex);
}

/*------------------------------------------------------------------------------*/

int MfxOmxSemaphore::Post(void)
{
    int res = 0;
    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if (0 == m_count++) VM_CHECK(res, pthread_cond_signal(&m_semaphore));

        VM_CHECK(res, pthread_mutex_unlock(&m_mutex));
    }
    return res;
}

/*------------------------------------------------------------------------------*/

int MfxOmxSemaphore::Wait(void)
{
    int res = 0;
    bool bError = false;

    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if ((0 == m_count) && (0 != pthread_cond_wait(&m_semaphore, &m_mutex)))
            bError = true;
        if (!bError) --m_count;

        res = pthread_mutex_unlock(&m_mutex);
    }
    return res;
}

/*------------------------------------------------------------------------------*/

int MfxOmxSemaphore::Wait(mfxU32 timeout)
{
    if (timeout == 0)
        return Wait();

    int res = 0;
    bool bError = false;
    int wait_res = 0;


    struct timespec tspec;
    struct timeval tval;
    unsigned long long micro_sec;

    gettimeofday(&tval, NULL);
    micro_sec = 1000 * timeout + tval.tv_usec;
    tspec.tv_sec = tval.tv_sec + (uint32_t)(micro_sec / 1000000);
    tspec.tv_nsec = (uint32_t)(micro_sec % 1000000) * 1000;

    res = pthread_mutex_lock(&m_mutex);
    if (!res)
    {
        if (0 == m_count)
        {
            wait_res = pthread_cond_timedwait(&m_semaphore, &m_mutex, &tspec);
            if (wait_res != 0 && wait_res != ETIMEDOUT)
                bError = true;
        }
        if (!bError) --m_count;

        res = pthread_mutex_unlock(&m_mutex);
        res = (wait_res == ETIMEDOUT) ? wait_res : res;
    }

    return res;
}

/*------------------------------------------------------------------------------*/
/*                              T H R E A D S                                   */
/*------------------------------------------------------------------------------*/

void* mfx_omx_thread_start(void* arg)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    MFX_OMX_AUTO_TRACE_P(arg);
    if (arg)
    {
        MfxOmxThread* thread = (MfxOmxThread*)arg;

        MFX_OMX_AUTO_TRACE_P((void*)thread->m_func);
        thread->m_func(thread->m_arg);
    }
    return NULL;
}

/*------------------------------------------------------------------------------*/

MfxOmxThread::MfxOmxThread(mfx_omx_thread_callback func, void* arg)
{
    m_func = func;
    m_arg = arg;

   m_thread = std::thread(mfx_omx_thread_start, this);
}

/*------------------------------------------------------------------------------*/

MfxOmxThread::~MfxOmxThread(void)
{
}

/*------------------------------------------------------------------------------*/

void MfxOmxThread::Wait(void)
{
    if(m_thread.joinable())
    {
        m_thread.join();
    }
}

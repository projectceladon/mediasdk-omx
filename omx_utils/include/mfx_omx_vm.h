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

#ifndef __MFX_OMX_VM_H__
#define __MFX_OMX_VM_H__

#include <thread>
#include "mfx_omx_types.h"

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

extern mfx_omx_so_handle mfx_omx_so_load(const char *file_name);

extern void mfx_omx_so_free(mfx_omx_so_handle handle);

extern void* mfx_so_get_addr(mfx_omx_so_handle handle, const char *func_name);

extern mfxU32 mfx_omx_get_cpu_num(void);

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

class MfxOmxMutex
{
public:
    MfxOmxMutex(void);
    ~MfxOmxMutex(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);
    bool Try(void);

private: // variables
    pthread_mutex_t m_mutex;

private: // functions
    MFX_OMX_CLASS_NO_COPY(MfxOmxMutex)
};

/*------------------------------------------------------------------------------*/

class MfxOmxAutoLock
{
public:
    MfxOmxAutoLock(MfxOmxMutex& mutex);
    ~MfxOmxAutoLock(void);

    mfxStatus Lock(void);
    mfxStatus Unlock(void);

private: // variables
    MfxOmxMutex& m_rMutex;
    bool m_bLocked;

private: // functions
    MFX_OMX_CLASS_NO_COPY(MfxOmxAutoLock)
};

/*------------------------------------------------------------------------------*/

class MfxOmxEvent
{
public:
    MfxOmxEvent(bool manual, bool state);
    ~MfxOmxEvent(void);

    int Signal(void);
    int Reset(void);
    int Wait(void);
    int TimedWait(mfxU32 msec);
private:
    bool m_manual;
    bool m_state;
    pthread_cond_t m_event;
    pthread_mutex_t m_mutex;

private: // functions
    MFX_OMX_CLASS_NO_COPY(MfxOmxEvent)
};

/*------------------------------------------------------------------------------*/

class MfxOmxSemaphore
{
public:
    MfxOmxSemaphore(mfxU32 count = 0);
    ~MfxOmxSemaphore(void);

    int Post(void);
    int Wait(void);
    int Wait(mfxU32 timeout);
//    void Reset(void);
private:
    mfxU32 m_count;
    pthread_cond_t m_semaphore;
    pthread_mutex_t m_mutex;

private: // functions
    MFX_OMX_CLASS_NO_COPY(MfxOmxSemaphore)
};

/*------------------------------------------------------------------------------*/

typedef unsigned int (*mfx_omx_thread_callback)(void*);

/*------------------------------------------------------------------------------*/

class MfxOmxThread
{
public:
    MfxOmxThread(mfx_omx_thread_callback func, void* arg);
    ~MfxOmxThread(void);

    void Wait(void);

protected:
    friend void* mfx_omx_thread_start(void* arg);

    std::thread m_thread;
    mfx_omx_thread_callback m_func;
    void* m_arg;

private: // functions
    MFX_OMX_CLASS_NO_COPY(MfxOmxThread)
};

#endif // #ifndef __MFX_OMX_VM_H__

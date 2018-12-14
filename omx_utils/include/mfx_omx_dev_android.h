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

#ifndef __MFX_OMX_DEV_ANDROID_H__
#define __MFX_OMX_DEV_ANDROID_H__

#ifdef LIBVA_SUPPORT

#include "mfx_omx_dev.h"

/*------------------------------------------------------------------------------*/

#define MFX_OMX_ANDROID_DISPLAY 0x18c34078

/*------------------------------------------------------------------------------*/

typedef unsigned int MfxOmxAndroidDisplay;

/*------------------------------------------------------------------------------*/

class MfxOmxDevAndroid : public MfxOmxDev
{
public:
    MfxOmxDevAndroid(mfxStatus &sts);
    virtual ~MfxOmxDevAndroid(void);

    virtual mfxStatus DevInit(void);
    virtual mfxStatus DevClose(void);

    virtual mfxStatus InitMfxSession(MFXVideoSession* session);
    virtual mfxFrameAllocator* GetFrameAllocator(void)
    {
        return m_pFrameAllocator;
    }

    virtual MfxOmxGrallocAllocator* GetGrallocAllocator(void)
    {
        return m_pGrallocAllocator;
    }

    virtual eMfxOmxHwType GetPlatformType(void);

    virtual OMX_U64 GetDriverVersion(void);

    virtual OMX_U32 GetDecProcessingRate(mfxVideoParam const & par);

protected:
    bool m_bInitialized;
    MfxOmxAndroidDisplay* m_Display;
    VADisplay m_vaDpy;

    MfxOmxVaapiFrameAllocator* m_pFrameAllocator;
    MfxOmxGrallocAllocator*    m_pGrallocAllocator;

    eMfxOmxHwType m_platformType;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxDevAndroid)
};

/*------------------------------------------------------------------------------*/

#endif // #ifdef LIBVA_SUPPORT

#endif // #ifndef __MFX_OMX_DEV_ANDROID_H__

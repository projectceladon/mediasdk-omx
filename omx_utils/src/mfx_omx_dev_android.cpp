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

#ifdef LIBVA_SUPPORT

#include "mfx_omx_dev_android.h"

#undef ANDROID
#include <va/va_android.h>
#include <va/va_backend.h>
#define ANDROID

#include <sys/ioctl.h>

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_dev_android"

/*------------------------------------------------------------------------------*/

#define I915_PARAM_CHIPSET_ID 4
#define DRM_I915_GETPARAM 0x06
#define DRM_IOCTL_BASE 'd'
#define DRM_COMMAND_BASE 0x40
#define DRM_IOWR(nr,type) _IOWR(DRM_IOCTL_BASE,nr,type)
#define DRM_IOCTL_I915_GETPARAM DRM_IOWR(DRM_COMMAND_BASE + DRM_I915_GETPARAM, drm_i915_getparam_t)

typedef struct {
    int device_id;
    eMfxOmxHwType platform;
} mfx_device_item;

const mfx_device_item listLegalDevIDs[] = {
    /* BXT */
    { 0x0A84, MFX_HW_BXT},
    { 0x0A85, MFX_HW_BXT},
    { 0x0A86, MFX_HW_BXT},
    { 0x0A87, MFX_HW_BXT},
    { 0x1A84, MFX_HW_BXT}, //BXT-PRO?
    /* APL */
    { 0x5A84, MFX_HW_BXT}
};

typedef struct drm_i915_getparam {
    int param;
    int *value;
} drm_i915_getparam_t;

/*------------------------------------------------------------------------------*/

MfxOmxDevAndroid::MfxOmxDevAndroid(mfxStatus &sts):
    m_bInitialized(false),
    m_Display(NULL),
    m_vaDpy(NULL),
    m_pFrameAllocator(NULL),
    m_pGrallocAllocator(NULL),
    m_platformType(MFX_HW_UNKNOWN)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    sts = MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

MfxOmxDevAndroid::~MfxOmxDevAndroid(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    DevClose();
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxDevAndroid::DevInit(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;
    VAStatus va_res = VA_STATUS_SUCCESS;
    int major_version = 0, minor_version = 0;

    if (m_bInitialized) mfx_res = MFX_ERR_UNKNOWN;
    else
    {
        if (MFX_ERR_NONE == mfx_res)
        {
            m_Display = (MfxOmxAndroidDisplay*)malloc(sizeof(MfxOmxAndroidDisplay));
            if (m_Display) *m_Display = MFX_OMX_ANDROID_DISPLAY;
            else mfx_res = MFX_ERR_MEMORY_ALLOC;
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            m_vaDpy = vaGetDisplay(m_Display);
            va_res = vaInitialize(m_vaDpy, &major_version, &minor_version);
            if (VA_STATUS_SUCCESS == va_res) MFX_OMX_LOG_INFO("Driver version is %s", vaQueryVendorString(m_vaDpy));
            else
            {
                MFX_OMX_LOG_ERROR("vaInitialize failed with an error 0x%X", va_res);
                mfx_res = MFX_ERR_UNKNOWN;
            }
        }

        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = MfxOmxVaapiFrameAllocator::Create(m_vaDpy, &m_pFrameAllocator);
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_AUTO_TRACE_MSG("Failed to create MfxOmxVaapiFrameAllocator");
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            mfx_res = MfxOmxGrallocAllocator::Create(&m_pGrallocAllocator);
            if (MFX_ERR_NONE != mfx_res)
            {
                MFX_OMX_AUTO_TRACE_MSG("Failed to create MfxOmxGrallocAllocator");
            }
        }
        if (MFX_ERR_NONE == mfx_res)
        {
            m_bInitialized = true;

            int fd = 0, i = 0, listSize = 0;
            int devID = 0;
            int ret = 0;
            drm_i915_getparam_t gp;
            VADisplayContextP pDisplayContext = NULL;
            VADriverContextP pDriverContext = NULL;

            pDisplayContext = (VADisplayContextP) m_vaDpy;
            pDriverContext  = pDisplayContext->pDriverContext;
            fd = *(int*) pDriverContext->drm_state;

            gp.param = I915_PARAM_CHIPSET_ID;
            gp.value = &devID;

            ret = ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);
            if (!ret)
            {
                listSize = (sizeof(listLegalDevIDs)/sizeof(mfx_device_item));
                for (i = 0; i < listSize; ++i)
                {
                    if (listLegalDevIDs[i].device_id == devID)
                    {
                        m_platformType = listLegalDevIDs[i].platform;
                        break;
                    }
                }
            }
        }
        else
        {
            MFX_OMX_DELETE(m_pFrameAllocator);
            MFX_OMX_FREE(m_Display);
            MFX_OMX_DELETE(m_pGrallocAllocator);
        }
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxDevAndroid::DevClose(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (m_bInitialized)
    {
        MFX_OMX_DELETE(m_pFrameAllocator);
        vaTerminate(m_vaDpy);
        MFX_OMX_FREE(m_Display);
        MFX_OMX_DELETE(m_pGrallocAllocator);
    }
    return MFX_ERR_NONE;
}

/*------------------------------------------------------------------------------*/

mfxStatus MfxOmxDevAndroid::InitMfxSession(MFXVideoSession* session)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    if (!session) mfx_res = MFX_ERR_NULL_PTR;
    if (MFX_ERR_NONE == mfx_res)
    { // Check if already initialized
        VADisplay dpy = NULL;
        mfx_res = session->GetHandle(static_cast<mfxHandleType>(MFX_HANDLE_VA_DISPLAY), (mfxHDL*)&dpy);
        if (MFX_ERR_NOT_FOUND == mfx_res && !dpy) mfx_res = MFX_ERR_NONE;
        if (MFX_ERR_NONE == mfx_res && dpy) mfx_res = MFX_WRN_VALUE_NOT_CHANGED;
    }

    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = session->SetHandle(static_cast<mfxHandleType>(MFX_HANDLE_VA_DISPLAY), (mfxHDL)m_vaDpy);
    }
    if (MFX_ERR_NONE == mfx_res)
    {
        mfx_res = session->SetFrameAllocator(m_pFrameAllocator);
    }
    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

/*------------------------------------------------------------------------------*/

eMfxOmxHwType MfxOmxDevAndroid::GetPlatformType(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();

    return m_platformType;
}

/*------------------------------------------------------------------------------*/

OMX_U64 MfxOmxDevAndroid::GetDriverVersion(void)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxU32 majorVersion = 0;
    mfxU32 minorVersion = 0;
    mfxU32 microVersion = 0;

    // 'X.X.XXXXX-ubit-Release', where XXXXX is a version
    const char* vendorString = vaQueryVendorString(m_vaDpy);
    if (vendorString)
    {
        char sep = '.';
        const char *str = NULL;

        majorVersion = atol(vendorString);

        if ((str = strchr(vendorString, sep)) != 0)
        {
            str++;
            if (str) minorVersion = atol(str);
        }
        if (str && ((str = strchr(str, sep)) != 0))
        {
            str++;
            if (str) microVersion = atol(str);
        }
    }

    MFX_OMX_AUTO_TRACE_I64(microVersion);
    return microVersion;
}

/*------------------------------------------------------------------------------*/

OMX_U32 MfxOmxDevAndroid::GetDecProcessingRate(mfxVideoParam const & par)
{
    mfxU32 processing_rate = 0;

    VAConfigID config = VA_INVALID_ID;
    VAConfigAttrib attrib[2];
    attrib[0].type = VAConfigAttribRTFormat;
    attrib[0].value = VA_RT_FORMAT_YUV420;
    attrib[1].type = VAConfigAttribDecSliceMode;
    attrib[1].value = VA_DEC_SLICE_MODE_NORMAL;

    VAProfile vaProfile;
    switch (par.mfx.CodecProfile)
    {
        case MFX_PROFILE_AVC_CONSTRAINED_BASELINE:
            vaProfile = VAProfileH264ConstrainedBaseline; break;
        case MFX_PROFILE_AVC_BASELINE:
            vaProfile = VAProfileH264Baseline; break;
        case MFX_PROFILE_AVC_MAIN:
            vaProfile = VAProfileH264Main; break;
        case MFX_PROFILE_AVC_HIGH:
            vaProfile = VAProfileH264High; break;
        default:
            vaProfile = VAProfileH264Baseline; break;
    }

    VAStatus vaSts = vaCreateConfig(
        m_vaDpy,
        vaProfile,
        VAEntrypointVLD,
        attrib,
        2,
        &config);
    if (VA_STATUS_SUCCESS == vaSts)
    {
        VAProcessingRateParameter proc_rate_buf = {};
        proc_rate_buf.proc_buf_dec.level_idc = par.mfx.CodecLevel;
        vaSts = vaQueryProcessingRate(m_vaDpy, config, &proc_rate_buf, &processing_rate);

        vaDestroyConfig(m_vaDpy, config);
    }

    return processing_rate;
}

#endif // #ifdef LIBVA_SUPPORT

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

#ifndef __MFX_OMX_SRF_IBUF_H__
#define __MFX_OMX_SRF_IBUF_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_buffers.h"
#include "mfx_omx_dev.h"

/*------------------------------------------------------------------------------*/

class MfxOmxInputSurfacesPool : public MfxOmxInputRefBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>
{
public:
    enum
    {
        MODE_LOAD_SWMEM   = 0x00000000, // Default value
        MODE_LOAD_MDBUF   = 0x00000001, // Load surface from MetadataBuffer
        MODE_LOAD_OPAQUE  = 0x00000002  // buffer is [4-byte type | 4-byte handle] (OMX_COLOR_FormatAndroidOpaque, see MetadataBufferType.h)
    };

public:
    MfxOmxInputSurfacesPool(mfxStatus &sts);
    virtual ~MfxOmxInputSurfacesPool(void);

    virtual void SetFile(FILE* file) { m_dbg_file = file; }

    /**
     * Initializes class instance. If input surfaces can't be mapped directly
     * (for example sizes are not properly aligned) then this class will perform
     * copying of the data to the proper surface.
     * @param[in] pInputFramesInfo parameters of actual input frames
     * @param[in] pMfxInfo Media SDK initialization parameters
     * @note We assume that caller of this function is responsible for the
     * parameters correctness in a way that pMfxInfo is obtained from pInputFramesInfo
     * and only width/height parameters changed (aligned).
     */
    virtual mfxStatus Init(
        mfxFrameInfo* pInputFramesInfo,
        mfxFrameInfo* pMfxInfo);
    /** Deinitializes class instance. */
    virtual void Close(void);
    /** Resets class instance to initialized state. */
    virtual mfxStatus Reset(void);

    /** Prepares surface for later usage. */
    virtual mfxStatus PrepareSurface(OMX_BUFFERHEADERTYPE* pBuffer);
    /** Releases surface. */
    virtual mfxStatus FreeSurface(OMX_BUFFERHEADERTYPE* pBuffer);
    /** Returns EOS status. */
    virtual bool WasEosReached(void) { return m_bEOS; }

    virtual mfxStatus UseBuffer(OMX_BUFFERHEADERTYPE* pBuffer, MfxOmxInputConfig config);
    // This is necessary to avoid warning : hides overloaded virtual function [-Woverloaded-virtual]
    using MfxOmxInputRefBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::UseBuffer;

    // MfxOmxBuffersPool<OMX_BUFFERHEADERTYPE> methods
    virtual mfxStatus ReleaseBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
    virtual mfxStatus IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked);

    mfxStatus SetMfxDevice(MfxOmxDev* dev);
    mfxStatus SetMode(const mfxU32 enabled);

    bool IsRepeatedFrame(void);

    void SetBlackFrame(buffer_handle_t handle) { m_blackFrame = handle; }

protected: // functions
    mfxStatus LoadSurface(OMX_BUFFERHEADERTYPE* pBuffer, MfxOmxInputConfig& config);
    mfxStatus LoadSurfaceHW(mfxU8 *data, mfxU32 length, mfxFrameSurface1* srf);
    mfxStatus LoadSurfaceSW(mfxU8 *data, mfxU32 length, mfxFrameSurface1* srf);

protected: // variables
    bool m_bInitialized;
    bool m_bEOS; // EOS flag
    MfxOmxDev* m_pDevice;
    mfxFrameInfo m_InputFramesInfo;
    mfxFrameInfo m_MfxFramesInfo;

    mfxU32 m_inputDataMode;
    buffer_handle_t m_blackFrame;
    FILE* m_dbg_file;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxInputSurfacesPool)
};

#endif // #ifndef __MFX_OMX_SRF_IBUF_H__

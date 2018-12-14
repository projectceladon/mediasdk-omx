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

#ifndef __MFX_OMX_SRF_POOL_H__
#define __MFX_OMX_SRF_POOL_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_buffers.h"
#include "mfx_omx_dev.h"

#include <list>

/*------------------------------------------------------------------------------*/

class MfxOmxSurfacesPool : public MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>
{
public:
    MfxOmxSurfacesPool(mfxU32 CodecId, mfxStatus &sts);
    virtual ~MfxOmxSurfacesPool(void);

    virtual void SetFile(FILE* file) { m_dbg_file = file; }

    // closes pool
    virtual mfxStatus Close(void);
    // reset pool
    virtual mfxStatus Reset(void);
    // initializes buffer for later usage
    virtual mfxStatus PrepareSurface(OMX_BUFFERHEADERTYPE* pBuffer, mfxFrameInfo* pFrameInfo, mfxMemId MemId);
    // displays current surface
    virtual void DisplaySurface(bool bErrorReportingEnabled, bool isReinit = false);

    virtual void SetFrameAllocator(mfxFrameAllocator* pFrameAllocator){ m_pFrameAllocator = pFrameAllocator; }
    virtual void SetMfxDevice(MfxOmxDev* device) { m_pDevice = device; }

     // sets frame allocator
    virtual void SetSurfacesAllocationMode(bool bOnFlySurfacesAllocation, bool bANWBufferInMetaData)
    {
        m_bOnFlySurfacesAllocation = bOnFlySurfacesAllocation;
        m_bANWBufferInMetaData = bANWBufferInMetaData;
    }

    // add arrived surface to the pool
    mfxStatus UseBuffer(OMX_BUFFERHEADERTYPE* pBuffer, mfxFrameInfo & mfx_info, bool bChangeOutputPortSettings);
    // This is necessary to avoid warning : hides overloaded virtual function [-Woverloaded-virtual]
    using MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1>::UseBuffer;

    // swap buffer info for particular OMX buffer
    mfxStatus SwapBufferInfo(OMX_BUFFERHEADERTYPE* pBuffer);

    // gets surfaces pitch
    virtual mfxU16 GetPitch(mfxFrameInfo* pFrameInfo);

    // MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxFrameSurface1> functions
    virtual mfxStatus IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked);

    virtual mfxStatus FillOutputErrorBuffer(OMX_VIDEO_ERROR_BUFFER* pErrorBuffer, OMX_U32 nErrorBufIndex);

    virtual mfxStatus SetHeaderErrors(mfxFrameSurface1* pSurfaces, mfxU32 isHeaderCorrupted);

    virtual void SetMaxErrorCount(mfxU32 maxErrors);

protected: // functions
    mfxStatus SetCommonErrors(MfxOmxBufferInfo* pAddBufInfo);
    OMX_VIDEO_ERROR_BUFFER* GetCurrentErrorItem(MfxOmxBufferInfo* pAddBufInfo);

protected: // variables
    mfxU32 m_codecId;
    // surfaces pitch
    mfxU16 m_nPitch;
    // frame allocator
    mfxFrameAllocator* m_pFrameAllocator;
    MfxOmxDev* m_pDevice;
    // onfly surfaces allocation enable/disable flag
    bool m_bOnFlySurfacesAllocation;
    bool m_bANWBufferInMetaData;
    // previous TimeStamp (for detection decrease in output pts)
    OMX_TICKS m_latestTS;

    std::list<MfxOmxVideoErrorBuffer*> m_errorList;
    std::list<MfxOmxBufferInfo*> m_BufferInfoPool;
    std::list<MfxOmxBufferSwapPair*> m_SwapPool;

    // debug file dumps
    FILE* m_dbg_file;

private:
    mfxStatus CheckBufferForLog(MfxOmxRing<OMX_BUFFERHEADERTYPE*>& buffersList, MfxOmxBufferInfo* pBuffInfo, bool& bUsed);
    mfxStatus FindAvailableSurface(OMX_BUFFERHEADERTYPE** ppBuffer, MfxOmxBufferInfo** ppAddBufInfo);

    MFX_OMX_CLASS_NO_COPY(MfxOmxSurfacesPool)
    mfxU32 m_maxErrorCount;
};

#endif // #ifndef __MFX_OMX_SRF_POOL_H__

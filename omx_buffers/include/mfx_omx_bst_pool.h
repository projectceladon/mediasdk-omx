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

#ifndef __MFX_OMX_BST_POOL_H__
#define __MFX_OMX_BST_POOL_H__

#include "mfx_omx_utils.h"
#include "mfx_omx_buffers.h"

/*------------------------------------------------------------------------------*/

class MfxOmxBitstreamsPool : public MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxBitstream>
{
public:
    MfxOmxBitstreamsPool(mfxU32 CodecId, mfxStatus &sts);
    virtual ~MfxOmxBitstreamsPool(void);

    /** Debug purpose function. Permits to enable dumping of encoded bitstream to the file. */
    virtual void SetFile(FILE* file) { m_dbg_file = file; }

    /** Initializes buffer for later usage. Function fills internal info for the buffer. */
    virtual mfxStatus PrepareBitstream(OMX_BUFFERHEADERTYPE* pBuffer, mfxInfoMFX* pMfxInfo);
    /** Sends given bitstream to downstream plug-in. */
    virtual void SendBitstream(OMX_BUFFERHEADERTYPE* pBuffer);

    // MfxOmxOutputBuffersPool<OMX_BUFFERHEADERTYPE, mfxBitstream> functions
    virtual mfxStatus UseBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
    virtual mfxStatus IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked);
    void SetRemovingSpsPps(bool bRemove);
    bool IsRemovingSPSPPSNeeded(void);

protected: //functions
    mfxStatus LoadBitstream(OMX_BUFFERHEADERTYPE* pBuffer);
    mfxStatus RemoveHeaders(mfxBitstream *Bitstream);
protected: // variables
    mfxU32 m_codecId;
    bool m_bCodecDataSent;
    bool m_bRemoveSPSPPS;
    /** Debug purposes file to which encoded bitstream can be dumped. */
    FILE* m_dbg_file;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxBitstreamsPool)
};

/*------------------------------------------------------------------------------*/

#endif // #ifndef __MFX_OMX_BST_POOL_H__

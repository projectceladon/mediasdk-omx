// Copyright (c) 2013-2018 Intel Corporation
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

#ifndef __MFX_OMX_ALLOCATOR_H__
#define __MFX_OMX_ALLOCATOR_H__

#include <list>
#include <functional>

#include "mfx_omx_utils.h"

struct MfxOmxFrameAllocResponse
{
    mfxU32 refcount;
    mfxFrameAllocResponse response;
};

class MfxOmxFrameAllocator: public mfxFrameAllocator
{
public:
    MfxOmxFrameAllocator();
    virtual ~MfxOmxFrameAllocator();

    virtual mfxStatus AllocFrames(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response);
    virtual mfxStatus FreeFrames(mfxFrameAllocResponse *response);

    virtual mfxStatus LockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;
    virtual mfxStatus UnlockFrame(mfxMemId mid, mfxFrameData *ptr) = 0;

    virtual mfxStatus GetFrameHDL(mfxMemId mid, mfxHDL *handle) = 0;

protected: //functions
    // checks if request is supported
    virtual mfxStatus CheckRequestType(mfxFrameAllocRequest *request);
    // frees memory attached to response
    virtual mfxStatus ReleaseResponse(mfxFrameAllocResponse *response) = 0;
    // allocates memory
    virtual mfxStatus AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response) = 0;

protected: // variables
    MfxOmxFrameAllocResponse m_DecoderResponse;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxFrameAllocator)
};

#endif // __MFX_OMX_ALLOCATOR_H__

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

#ifndef __MFX_OMX_BST_IBUF_H__
#define __MFX_OMX_BST_IBUF_H__

#include <memory>

#ifdef HDR_SEI_PAYLOAD
#include <map>
#endif

#include "mfx_omx_utils.h"
#include "mfx_omx_buffers.h"

/*------------------------------------------------------------------------------*/

enum MfxOmxFrameConstructorType
{
    MfxOmxFC_None,
    MfxOmxFC_AVC,
    MfxOmxFC_HEVC,
    MfxOmxFC_VC1,
    MfxOmxFC_VP8,
    MfxOmxFC_VP9
};

/*------------------------------------------------------------------------------*/

enum MfxOmxBitstreamState
{
    MfxOmxBS_HeaderAwaiting = 0,
    MfxOmxBS_HeaderCollecting = 1,
    MfxOmxBS_HeaderWaitSei = 2,
    MfxOmxBS_HeaderObtained = 3,
    MfxOmxBS_Resetting = 4,
};

/*------------------------------------------------------------------------------*/

class IMfxOmxFrameConstructor
{
public:
    virtual ~IMfxOmxFrameConstructor(void) {}

    virtual void SetFiles(FILE* file, FILE* fc_file) = 0;

    // inits frame constructor
    virtual mfxStatus Init(mfxU16 profile, mfxFrameInfo fr_info) = 0;
    // loads next portion of data; fc may directly use that buffer or append header, etc.
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame) = 0;
    // forces synchronisation of current buffer with internal one
    virtual mfxStatus Sync(void) = 0;
    // unloads perviously sent buffer, copyis data to internal buffer if needed
    virtual mfxStatus Unload(void) = 0;
    // resets frame constructor
    virtual mfxStatus Reset(void) = 0;
    // cleans up resources
    virtual void Close(void) = 0;
    // gets bitstream with data
    virtual mfxBitstream* GetMfxBitstream(void) = 0;
    // notifies that end of stream reached
    virtual void SetEosMode(bool bEos) = 0;
    // returns EOS status
    virtual bool WasEosReached(void) = 0;
    // save current SPS/PPS
    virtual mfxStatus SaveHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS, bool isReset) = 0;
    // detect interlaced content
    virtual bool IsSetInterlaceFlag(bool * bInterlaced) = 0;

#ifdef HDR_SEI_PAYLOAD
    // get saved SEI (right now only for HEVC 10 bit SeiHDRStaticInfo)
    virtual mfxPayload* GetSei(mfxU32 /*type*/) = 0;
#endif

    // function which should be extended (framework specific)

    // releases sample from which bitstream data arrived
    virtual void ReleaseSample(void) = 0;

};

class MfxOmxFrameConstructor : public IMfxOmxFrameConstructor
{
public:
    MfxOmxFrameConstructor(mfxStatus &sts);
    virtual ~MfxOmxFrameConstructor(void);

    virtual void SetFiles(FILE* file, FILE* fc_file) { m_dbg_file = file; m_dbg_file_fc = fc_file; }

    // inits frame constructor
    virtual mfxStatus Init(mfxU16 profile, mfxFrameInfo fr_info);
    // loads next portion of data; fc may directly use that buffer or append header, etc.
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);
    // forces synchronisation of current buffer with internal one
    virtual mfxStatus Sync(void) { return Unload(); }
    // unloads perviously sent buffer, copyis data to internal buffer if needed
    virtual mfxStatus Unload(void);
    // resets frame constructor
    virtual mfxStatus Reset(void);
    // cleans up resources
    virtual void Close(void) { Reset(); }
    // gets bitstream with data
    virtual mfxBitstream* GetMfxBitstream(void);
    // notifies that end of stream reached
    virtual void SetEosMode(bool bEos) { m_bEOS = bEos; }
    // returns EOS status
    virtual bool WasEosReached(void) { return m_bEOS; }
    // save current SPS/PPS
    virtual mfxStatus SaveHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS, bool isReset)
    {
        MFX_OMX_UNUSED(pSPS);
        MFX_OMX_UNUSED(pPPS);
        MFX_OMX_UNUSED(isReset);
        return MFX_ERR_NONE;
    }
    // detect interlaced content
    virtual bool IsSetInterlaceFlag(bool * bInterlaced)
    {
        *bInterlaced = false;
        return false;
    }

#ifdef HDR_SEI_PAYLOAD
    // get saved SEI (right now only for HEVC 10 bit SeiHDRStaticInfo)
    virtual mfxPayload* GetSei(mfxU32 /*type*/) {return nullptr;}
#endif


    // function which should be extended (framework specific)

    // releases sample from which bitstream data arrived
    virtual void ReleaseSample(void) { return; }

protected: // functions
    virtual mfxStatus LoadHeader(mfxU8* data, mfxU32 size, bool b_header);
    virtual mfxStatus Load_None(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);

    // increase buffer capacity with saving of buffer content (realloc)
    mfxStatus BstBufRealloc(mfxU32 add_size);
    // increase buffer capacity without saving of buffer content (free/malloc)
    mfxStatus BstBufMalloc (mfxU32 new_size);
    // cleaning up of internal buffers
    mfxStatus BstBufSync(void);

protected: // variables
    // parameters which define FC behavior
    MfxOmxBitstreamState m_bs_state;
    mfxFrameInfo m_fr_info;
    mfxU16 m_profile;

    // mfx bistreams:
    // pointer to current bitstream
    mfxBitstream* m_pBst;
    // saved stream header to be returned after seek if no new header will be found
    mfxBitstream m_BstHeader;
    // buffered data: seq header or remained from previos sample
    mfxBitstream m_BstBuf;
    // data from input sample (case when buffering and copying is not needed)
    mfxBitstream m_BstIn;

    // EOS flag
    bool m_bEOS;

    // some statistics:
    mfxU32 m_nBstBufReallocs;
    mfxU32 m_nBstBufCopyBytes;

    // debug file dumps
    FILE* m_dbg_file;
    FILE* m_dbg_file_fc;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxFrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxAVCFrameConstructor : public MfxOmxFrameConstructor
{
public:
    MfxOmxAVCFrameConstructor(mfxStatus &sts);
    virtual ~MfxOmxAVCFrameConstructor(void);

    // save current SPS/PPS
    virtual mfxStatus SaveHeaders(mfxBitstream *pSPS, mfxBitstream *pPPS, bool isReset);
    // detect interlaced content
    virtual bool IsSetInterlaceFlag(bool * bInterlaced);

#ifdef HDR_SEI_PAYLOAD
    // get saved SEI (right now only for HEVC 10 bit SeiHDRStaticInfo)
    virtual mfxPayload* GetSei(mfxU32 /*type*/) {return nullptr;}
#endif

protected: // functions
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);
    virtual mfxStatus LoadHeader(mfxU8* data, mfxU32 size, bool b_header);

#ifdef HDR_SEI_PAYLOAD
    // save current SEI
    virtual mfxStatus SaveSEI(mfxBitstream */*pSEI*/) {return MFX_ERR_NONE;}
#endif

    virtual mfxStatus FindHeaders(mfxU8* data, mfxU32 size, bool &bFoundSps, bool &bFoundPps, bool &bFoundSei);
    virtual mfxI32    FindStartCode(mfxU8 * (&pb), mfxU32 & size, mfxI32 & startCodeSize);
    virtual bool      isSPS(mfxI32 code) {return NAL_UT_AVC_SPS == code;}
    virtual bool      isPPS(mfxI32 code) {return NAL_UT_AVC_PPS == code;}

#ifdef HDR_SEI_PAYLOAD
    virtual bool      isSEI(mfxI32 /*code*/) {return false;}
    virtual bool      IsNeedWaitSEI(mfxI32 /*code*/) {return false;}
#endif

protected: // variables
    const static mfxU32 NAL_UT_AVC_SPS = 7;
    const static mfxU32 NAL_UT_AVC_PPS = 8;

    mfxBitstream m_SPS;
    mfxBitstream m_PPS;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxAVCFrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxHEVCFrameConstructor : public MfxOmxAVCFrameConstructor
{
public:
    MfxOmxHEVCFrameConstructor(mfxStatus &sts):
    MfxOmxAVCFrameConstructor(sts)
    {
        MFX_OMX_AUTO_TRACE_FUNC();
    }
    virtual ~MfxOmxHEVCFrameConstructor(void)
    {
        MFX_OMX_AUTO_TRACE_FUNC();

#ifdef HDR_SEI_PAYLOAD
        for (auto &sei : SEIMap)
        {
            MFX_OMX_FREE(sei.second.Data);
        }
#endif
    }

#ifdef HDR_SEI_PAYLOAD
    // get saved SEI (right now only for HEVC 10 bit SeiHDRStaticInfo)
    virtual mfxPayload* GetSei(mfxU32 type);

    const static mfxU32 SEI_MASTERING_DISPLAY_COLOUR_VOLUME = 137;
    const static mfxU32 SEI_CONTENT_LIGHT_LEVEL_INFO = 144;
#endif

protected: // functions
    virtual mfxI32 FindStartCode(mfxU8 * (&pb), mfxU32 & size, mfxI32 & startCodeSize);

#ifdef HDR_SEI_PAYLOAD
    // save current SEI
    virtual mfxStatus SaveSEI(mfxBitstream *pSEI);
#endif

    virtual bool   isSPS(mfxI32 code) {return NAL_UT_HEVC_SPS == code;}
    virtual bool   isPPS(mfxI32 code) {return NAL_UT_HEVC_PPS == code;}

#ifdef HDR_SEI_PAYLOAD
    virtual bool   isSEI(mfxI32 code) {return NAL_UT_HEVC_SEI == code;}
    virtual bool   IsNeedWaitSEI(mfxI32 code) { return NAL_UT_CODED_SLICEs.end() == std::find(NAL_UT_CODED_SLICEs.begin(), NAL_UT_CODED_SLICEs.end(), code);}
#endif

protected: // variables
    const static mfxU32 NAL_UT_HEVC_SPS = 33;
    const static mfxU32 NAL_UT_HEVC_PPS = 34;

#ifdef HDR_SEI_PAYLOAD
    const static mfxU32 NAL_UT_HEVC_SEI = 39;
    const static std::vector<mfxU32> NAL_UT_CODED_SLICEs;

    std::map<mfxU32, mfxPayload> SEIMap;
#endif

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxHEVCFrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxVC1FrameConstructor : public MfxOmxFrameConstructor
{
public:
    MfxOmxVC1FrameConstructor(mfxStatus &sts);
    virtual ~MfxOmxVC1FrameConstructor(void);

protected: // functions
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVC1FrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxVP8FrameConstructor : public MfxOmxFrameConstructor
{
public:
    MfxOmxVP8FrameConstructor(mfxStatus &sts);
    virtual ~MfxOmxVP8FrameConstructor(void);

protected: // functions
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);

protected: // variables
    bool m_bFirstSample;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVP8FrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxVP9FrameConstructor : public MfxOmxFrameConstructor
{
public:
    MfxOmxVP9FrameConstructor(mfxStatus &sts);
    virtual ~MfxOmxVP9FrameConstructor(void);

protected: // functions
    virtual mfxStatus Load(mfxU8* data, mfxU32 size, mfxU64 pts, bool b_header, bool bCompleteFrame);

protected: // variables
    bool m_bFirstSample;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxVP9FrameConstructor)
};

/*------------------------------------------------------------------------------*/

class MfxOmxFrameConstructorFactory
{
public:
    static IMfxOmxFrameConstructor* CreateFrameConstructor(MfxOmxFrameConstructorType fc_type, mfxStatus &sts);
};

/*------------------------------------------------------------------------------*/

class MfxOmxBitstream : public MfxOmxInputBuffersPool<OMX_BUFFERHEADERTYPE>
{
friend class BitstreamLoader;

public:
    MfxOmxBitstream(MfxOmxFrameConstructorType fc_type, mfxStatus &sts);
    virtual ~MfxOmxBitstream(void);

    virtual mfxStatus InitFrameConstructor(mfxU16 profile, mfxFrameInfo fr_info);

    virtual mfxStatus Reset(void);

    // MfxOmxInputBuffersPool<OMX_BUFFERHEADERTYPE> methods
    virtual mfxStatus IsBufferLocked(OMX_BUFFERHEADERTYPE* pBuffer, bool& bIsLocked);

    virtual IMfxOmxFrameConstructor* GetFrameConstructor(void) { return m_frameConstructor.get(); }

protected: // functions
    // load/unload bitstream data
    virtual mfxStatus LoadOmxBuffer(OMX_BUFFERHEADERTYPE* pBuffer);
    virtual mfxStatus Unload(void);

    // releases sample from which bitstream data arrived
    virtual void ReleaseSample(void);

protected: // variables
    //frameConstructor object
    std::auto_ptr<IMfxOmxFrameConstructor> m_frameConstructor;

    // current buffer (sample)
    OMX_BUFFERHEADERTYPE* m_pBuffer;

private:
    MFX_OMX_CLASS_NO_COPY(MfxOmxBitstream)
};

/*------------------------------------------------------------------------------*/

class BitstreamLoader
{
public:
    BitstreamLoader(MfxOmxBitstream *pOmxBitstream) :
        m_pOmxBitstream(pOmxBitstream),
        m_bIsBufferLoaded(false)
    {
        MFX_OMX_AUTO_TRACE_FUNC();
    }
    virtual ~BitstreamLoader(void)
    {
        MFX_OMX_AUTO_TRACE_FUNC();
        LoadBuffer(NULL);
    }

    mfxStatus LoadBuffer(OMX_BUFFERHEADERTYPE* pBuffer)
    {
        MFX_OMX_AUTO_TRACE_FUNC();
        mfxStatus mfx_res = MFX_ERR_NONE;

        if (m_bIsBufferLoaded)
        {
            mfx_res = m_pOmxBitstream->Unload();
            m_bIsBufferLoaded = false;
        }
        if (pBuffer)
        {
            if (MFX_ERR_NONE == mfx_res)
            {
                mfx_res = m_pOmxBitstream->LoadOmxBuffer(pBuffer);
                m_bIsBufferLoaded = true;
            }
        }

        MFX_OMX_AUTO_TRACE_I32(mfx_res);
        return mfx_res;
    }

protected: // variables
    MfxOmxBitstream *m_pOmxBitstream;
    bool m_bIsBufferLoaded;

private:
    MFX_OMX_CLASS_NO_COPY(BitstreamLoader)
};

#endif // #ifndef __MFX_OMX_BST_IBUF_H__

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

/*------------------------------------------------------------------------------*/

#undef MFX_OMX_MODULE_NAME
#define MFX_OMX_MODULE_NAME "mfx_omx_utils"

/*------------------------------------------------------------------------------*/

NalUnit GetNalUnit(mfxU8 * begin, mfxU8 * end)
{
    for (; begin < end - 5; ++begin)
    {
        if ((begin[0] == 0 && begin[1] == 0 && begin[2] == 1) ||
        (begin[0] == 0 && begin[1] == 0 && begin[2] == 0 && begin[3] == 1))
        {
            mfxU8 numZero = (begin[2] == 1 ? 2 : 3);

            for (mfxU8 * next = begin + 4; next < end - 4; ++next)
            {
                if (next[0] == 0 && next[1] == 0 && next[2] == 1)
                {
                    if (*(next - 1) == 0)
                    {
                        --next;
                    }
                    return NalUnit(begin, next, numZero);
                }
            }
            return NalUnit(begin, end, numZero);
        }
    }

    return NalUnit();
}

/*------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*------------------------------------------------------------------------------*/

mfxSyncPoint* g_NilSyncPoint = NULL;

mfxVersion g_MfxVersion = { {MFX_VERSION_MINOR, MFX_VERSION_MAJOR} };

mfxU32 g_OmxLogLevel = 0;

/*------------------------------------------------------------------------------*/

size_t mfx_omx_dump(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t ret = 0;
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
    if (stream)
    {
       ret = fwrite(ptr, size, nmemb, stream);
       fflush(stream);
    }
#else
    MFX_OMX_UNUSED(ptr);
    MFX_OMX_UNUSED(size);
    MFX_OMX_UNUSED(nmemb);
    MFX_OMX_UNUSED(stream);
#endif
    return ret;
}

/*------------------------------------------------------------------------------*/

void mfx_omx_dump_YUV_from_NV12_data(FILE* f, mfxFrameData* pData, mfxFrameInfo* info, mfxU32 p)
{
    MFX_OMX_AUTO_TRACE_FUNC();
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
    mfxU32 i = 0, j = 0;
    mfxU32 cx = 0, cy = 0, cw = 0, ch = 0;

    if (f && pData && info)
    {
        cx = info->CropX;
        cy = info->CropY;
        cw = info->CropW;
        ch = info->CropH;
        // dumping Y
        for (i = 0; i < ch; ++i)
        {
            fwrite(pData->Y + cx + (cy + i)*p, 1, cw, f);
        }
        fflush(f);
        // dumping U
        for (i = 0; i < ch/2; ++i)
        for (j = 0; j < cw/2; ++j)
        {
            fwrite(pData->U + p*i + 2*j, 1, 1, f);
        }
        fflush(f);
        // dumping V
        if (!pData->V)
        {
            pData->V = pData->U + 1;
        }
        for (i = 0; i < ch/2; ++i)
        for (j = 0; j < cw/2; ++j)
        {
            fwrite(pData->V + p*i + 2*j, 1, 1, f);
        }
        fflush(f);
    }
#else
    MFX_OMX_UNUSED(f);
    MFX_OMX_UNUSED(pData);
    MFX_OMX_UNUSED(info);
    MFX_OMX_UNUSED(p);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_dump_YUV_from_P010_data(FILE* f, mfxFrameData * pData, mfxFrameInfo* pInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
    mfxU32 i = 0;

    if (f && pData && pInfo)
    {
        mfxI32 crop_x = pInfo->CropX << 1; // sample - two bytes
        mfxI32 crop_y = pInfo->CropY >> 1;
        mfxU32 pitch = pData->PitchLow + ((mfxU32)pData->PitchHigh << 16);

        for (i = 0; i < pInfo->CropH; i++)
        {
            fwrite(pData->Y + (pInfo->CropY * pitch + crop_x)+ i * pitch, 1, pInfo->CropW * 2, f);
        }
        fflush(f);

        crop_y >>= 1;
        for (i = 0; i < pInfo->CropH / 2; i++)
        {
            fwrite(pData->UV + (crop_y*pitch + crop_x) + i * pitch, 1, pInfo->CropW * 2, f);
        }
        fflush(f);
    }
#else
    MFX_OMX_UNUSED(f);
    MFX_OMX_UNUSED(pData);
    MFX_OMX_UNUSED(pInfo);
#endif
}

/*------------------------------------------------------------------------------*/

void mfx_omx_dump_RGB_from_RGB4_data(FILE* f, mfxFrameData* pData, mfxFrameInfo* pInfo, mfxU32 pitch)
{
    MFX_OMX_AUTO_TRACE_FUNC();
#if MFX_OMX_DEBUG_DUMP == MFX_OMX_YES
    mfxI32 i, h, w;
    mfxU8* ptr;

    if (f && pData && pInfo)
    {
        if (pInfo->CropH > 0 && pInfo->CropW > 0)
        {
            w = pInfo->CropW;
            h = pInfo->CropH;
        }
        else
        {
            w = pInfo->Width;
            h = pInfo->Height;
        }
        ptr = MFX_OMX_MIN( MFX_OMX_MIN(pData->R, pData->G), pData->B);
        ptr = ptr + pInfo->CropX + pInfo->CropY * pitch;

        for(i = 0; i < h; i++)
        {
            fwrite(ptr + i * pitch, 1, 4*w, f);
        }
        fflush(f);
    }
#else
    MFX_OMX_UNUSED(f);
    MFX_OMX_UNUSED(pData);
    MFX_OMX_UNUSED(pInfo);
    MFX_OMX_UNUSED(pitch);
#endif
}

void mfx_omx_copy_nv12(mfxFrameSurface1* pDst, mfxFrameSurface1* pSrc)
{
    mfxU32 i;
    const mfxU32 width = std::min(pSrc->Info.Width, pDst->Info.Width);
    const mfxU32 height = std::min(pSrc->Info.Height, pDst->Info.Height);

    for (i = 0; i < height/2; ++i)
    {
        // copying Y
        uint8_t *src = pSrc->Data.Y + i * pSrc->Data.Pitch;
        uint8_t *dst = pDst->Data.Y + i * pDst->Data.Pitch;
        std::copy(src, src + width, dst);

        // copying UV
        src = pSrc->Data.UV + i * pSrc->Data.Pitch;
        dst = pDst->Data.UV + i * pDst->Data.Pitch;
        std::copy(src, src + width, dst);
    }
    for (i = height/2; i < height; ++i)
    {
        // copying Y (remained data)
        uint8_t *src = pSrc->Data.Y + i * pSrc->Data.Pitch;
        uint8_t *dst = pDst->Data.Y + i * pDst->Data.Pitch;
        std::copy(src, src + width, dst);
    }
}

/*------------------------------------------------------------------------------*/

mfxF64 mfx_omx_get_framerate(mfxU32 fr_n, mfxU32 fr_d)
{
    if (fr_n && fr_d) return (mfxF64)fr_n / (mfxF64)fr_d;
    return 0.0;
}

/*------------------------------------------------------------------------------*/

void mfx_omx_framerate_norm(mfxU32& fr_n, mfxU32& fr_d)
{
    if (!fr_n || !fr_d) return;

    mfxF64 f_fr = (mfxF64)fr_n / (mfxF64)fr_d;
    mfxU32 i_fr = (mfxU32)(f_fr + .5);

    if (fabs(i_fr - f_fr) < 0.0001)
    {
        fr_n = i_fr;
        fr_d = 1;
        return;
    }

    i_fr = (mfxU32)(f_fr * 1.001 + .5);
    if (fabs(i_fr * 1000 - f_fr * 1001) < 10)\
    {
        fr_n = i_fr * 1000;
        fr_d = 1001;
        return;
    }

    fr_n = (mfxU32)(f_fr * 10000 + .5);
    fr_d = 10000;
    return;
}

/*------------------------------------------------------------------------------*/

bool mfx_omx_are_framerates_equal(mfxU32 fr1_n, mfxU32 fr1_d,
                                  mfxU32 fr2_n, mfxU32 fr2_d)
{
    mfx_omx_framerate_norm(fr1_n, fr1_d);
    mfx_omx_framerate_norm(fr2_n, fr2_d);
    return (fr1_n == fr2_n) && (fr1_d == fr2_d);
}

/*------------------------------------------------------------------------------*/

void mfx_omx_reset_bitstream(mfxBitstream* pBitstream)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    if (pBitstream)
    {
        pBitstream->TimeStamp = MFX_OMX_TIME_STAMP_INVALID;
        pBitstream->DataLength = 0;
        pBitstream->DataOffset = 0;
        pBitstream->PicStruct = MFX_PICSTRUCT_UNKNOWN;
        pBitstream->FrameType = 0;
        pBitstream->DataFlag = 0;
    }
}

/*------------------------------------------------------------------------------*/

mfxStatus mfx_omx_get_metadatabuffer_info(mfxU8* data, mfxU32 size, MetadataBuffer* pInfo)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    mfxStatus mfx_res = MFX_ERR_NONE;

    MFX_OMX_AUTO_TRACE_I32(size);

    if (!data || !size || !pInfo)
        mfx_res = MFX_ERR_NULL_PTR;

    if (mfx_res == MFX_ERR_NONE)
    {
        MfxMetadataBufferType bufferType = *(reinterpret_cast<MfxMetadataBufferType*>(data));

        if (MfxMetadataBufferTypeGrallocSource == bufferType)
        {
            if (size < sizeof(android::VideoGrallocMetadata))
                mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
            else
            {
                pInfo->type = bufferType;
                android::VideoGrallocMetadata &grallocMeta = *(reinterpret_cast<android::VideoGrallocMetadata*>(data));
                pInfo->handle = grallocMeta.pHandle;
            }
        }
        else if (MfxMetadataBufferTypeANWBuffer == bufferType)
        {
            if (size < sizeof(android::VideoNativeMetadata))
                mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
            else
            {
                pInfo->type = bufferType;
                android::VideoNativeMetadata &nativeMeta = *(reinterpret_cast<android::VideoNativeMetadata*>(data));
                ANativeWindowBuffer *buffer = nativeMeta.pBuffer;
                pInfo->anw_buffer = buffer;
                pInfo->handle = buffer->handle;
                pInfo->pFenceFd = &nativeMeta.nFenceFd;
            }
        }
        else if (MfxMetadataBufferTypeNativeHandleSource == bufferType)
        {
            if (size < sizeof(android::VideoNativeHandleMetadata))
                 mfx_res = MFX_ERR_NOT_ENOUGH_BUFFER;
            else
            {
                pInfo->type = bufferType;
                android::VideoNativeHandleMetadata &nativeHandleMeta = *(reinterpret_cast<android::VideoNativeHandleMetadata*>(data));
                pInfo->handle = nativeHandleMeta.pHandle;
            }
        }
        else
            mfx_res = MFX_ERR_UNSUPPORTED;
    }

    MFX_OMX_AUTO_TRACE_I32(mfx_res);
    return mfx_res;
}

bool operator ==(NalUnit const & left, NalUnit const & right)
{
    return left.begin == right.begin && left.end == right.end;
}

bool operator !=(NalUnit const & left, NalUnit const & right)
{
    return !(left == right);
}

/*------------------------------------------------------------------------------*/

buffer_handle_t GetGrallocHandle(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    buffer_handle_t handle = NULL;

    if (pBuffer && pBuffer->pBuffer)
    {
        MetadataBuffer buffer;
        MFX_OMX_ZERO_MEMORY(buffer);

        mfxStatus mfx_res = mfx_omx_get_metadatabuffer_info(pBuffer->pBuffer + pBuffer->nOffset, pBuffer->nFilledLen ? pBuffer->nFilledLen : pBuffer->nAllocLen, &buffer);
        if (MFX_ERR_NONE == mfx_res) handle = (buffer_handle_t)buffer.handle;
    }

    MFX_OMX_AUTO_TRACE_P(handle);
    return handle;
}

MfxMetadataBufferType GetMetadataType(OMX_BUFFERHEADERTYPE* pBuffer)
{
    MFX_OMX_AUTO_TRACE_FUNC();
    MfxMetadataBufferType type = MfxMetadataBufferTypeInvalid;

    if (pBuffer && pBuffer->pBuffer)
    {
        MetadataBuffer buffer;
        MFX_OMX_ZERO_MEMORY(buffer);

        mfxStatus mfx_res = mfx_omx_get_metadatabuffer_info(pBuffer->pBuffer + pBuffer->nOffset, pBuffer->nFilledLen ? pBuffer->nFilledLen : pBuffer->nAllocLen, &buffer);
        if (MFX_ERR_NONE == mfx_res) type = buffer.type;
    }

    return type;
}

bool IsAvcHighProfile(mfxU32 profile)
{
    return
        profile == MFX_PROFILE_AVC_HIGH ||
        profile == MFX_PROFILE_AVC_CONSTRAINED_HIGH ||
        profile == MFX_PROFILE_AVC_PROGRESSIVE_HIGH;
}

mfxU32 GetMaxBufferSize(mfxVideoParam const & par)
{
    mfxU32 brFactor = IsAvcHighProfile(par.mfx.CodecProfile) ? 1500 : 1200;

    mfxU16 level = par.mfx.CodecLevel;
    if (level == MFX_LEVEL_UNKNOWN)
        level = MFX_LEVEL_AVC_52;

    switch (level)
    {
    case MFX_LEVEL_AVC_1 : return    175 * brFactor;
    case MFX_LEVEL_AVC_1b: return    350 * brFactor;
    case MFX_LEVEL_AVC_11: return    500 * brFactor;
    case MFX_LEVEL_AVC_12: return   1000 * brFactor;
    case MFX_LEVEL_AVC_13: return   2000 * brFactor;
    case MFX_LEVEL_AVC_2 : return   2000 * brFactor;
    case MFX_LEVEL_AVC_21: return   4000 * brFactor;
    case MFX_LEVEL_AVC_22: return   4000 * brFactor;
    case MFX_LEVEL_AVC_3 : return  10000 * brFactor;
    case MFX_LEVEL_AVC_31: return  14000 * brFactor;
    case MFX_LEVEL_AVC_32: return  20000 * brFactor;
    case MFX_LEVEL_AVC_4 : return  25000 * brFactor;
    case MFX_LEVEL_AVC_41: return  62500 * brFactor;
    case MFX_LEVEL_AVC_42: return  62500 * brFactor;
    case MFX_LEVEL_AVC_5 : return 135000 * brFactor;
    case MFX_LEVEL_AVC_51: return 240000 * brFactor;
    case MFX_LEVEL_AVC_52: return 240000 * brFactor;
    default: return 0;
    }
}

bool IsHRDBasedBRCMethod(mfxU16  RateControlMethod)
{
    return  RateControlMethod != MFX_RATECONTROL_CQP && RateControlMethod != MFX_RATECONTROL_AVBR &&
            RateControlMethod != MFX_RATECONTROL_ICQ &&
            RateControlMethod != MFX_RATECONTROL_LA && RateControlMethod != MFX_RATECONTROL_LA_ICQ;
}

const mfxU32 DEFAULT_CPB_IN_SECONDS = 2000;
const mfxU32 MAX_BITRATE_RATIO = mfxU32(1.5 * 1000);

mfxU32 GetBufferSize(mfxVideoParam const & par)
{
    mfxU32 bufferSize = 0;

    if (MFX_CODEC_AVC == par.mfx.CodecId)
    {
        mfxU32 maxKbps = 0;
        if (par.mfx.RateControlMethod == MFX_RATECONTROL_VBR ||
            par.mfx.RateControlMethod == MFX_RATECONTROL_VCM ||
            par.mfx.RateControlMethod == MFX_RATECONTROL_LA_HRD)
        {
            mfxU32 maxBps = par.mfx.TargetKbps * MAX_BITRATE_RATIO;
            maxKbps = mfxU32(MFX_OMX_MIN(maxBps / 1000, UINT_MAX));
        }
        else if (par.mfx.RateControlMethod == MFX_RATECONTROL_CBR)
        {
            maxKbps = par.mfx.TargetKbps;
        }

        mfxU32 bufferSizeInBits = MFX_OMX_MIN(
            GetMaxBufferSize(par),
            maxKbps * DEFAULT_CPB_IN_SECONDS);

        bufferSize = !IsHRDBasedBRCMethod(par.mfx.RateControlMethod)
            ? mfxU32(MFX_OMX_MIN(UINT_MAX, par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height * 3 / 2))
            : bufferSizeInBits / 8;
    }
    else
        bufferSize = mfxU32(MFX_OMX_MIN(UINT_MAX, par.mfx.FrameInfo.Width * par.mfx.FrameInfo.Height * 3 / 2));

    return bufferSize;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

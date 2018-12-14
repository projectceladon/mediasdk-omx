// Copyright (c) 2016-2018 Intel Corporation
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

#ifdef MFX_RESOURCES_LIMIT

#ifndef __MFX_OMX_RESOURCE_MANAGER_H__
#define __MFX_OMX_RESOURCE_MANAGER_H__

#include <vector>
#include <string>
#include <mutex>

enum class ResolutionType_e
{
    Resolution_NONE = 0,
    Resolution_480,
    Resolution_720,
    Resolution_1080,
    Resolution_2K,
    Resolution_4K,
    Resolution_MAX
};

enum class CodecType_e
{
    CODEC_TYPE_NONE = 0,
    CODEC_TYPE_VP8,
    CODEC_TYPE_VP9,
    CODEC_TYPE_HEVC,
    CODEC_TYPE_AVC,
    CODEC_TYPE_MP2,
    CODEC_TYPE_MAX
};

using CodecInfo = struct
{
    CodecType_e         codecType;
    bool                isEncoder;
    bool                isSecured;
    ResolutionType_e    resolutionType;
    unsigned int        frameRate;
};

using CodecLimitInfo = struct
{
    CodecInfo       codecInfo;
    unsigned int    maxLimit;
    unsigned int    currentLimit;
};

class MfxOmxComponentManager final
{
public:
    ~MfxOmxComponentManager() {};
    static MfxOmxComponentManager& GetInstanse()
    {
        static MfxOmxComponentManager pInstance;
        return pInstance;
    }

    // Configuration operations
    bool    LoadConfiguration(const std::string& configFilePath);
    bool    IsResourceAvailable();
    void    UpdateComponentTable(OMX_COMPONENTTYPE* pComponent);

private:
    void    SetDefaultConfiguration() noexcept;
    bool    CheckComponent(CodecInfo);
    bool    IsResourceExist(CodecInfo info);
    bool    IsSameResources(CodecLimitInfo source, CodecInfo target);

    //fill after call LoadConfiguration function
    std::vector<CodecLimitInfo>  mResources{};

    //fill every time in the GetHandle function
    std::vector<CodecInfo>  mActiveResources{};

    unsigned int            mCurrNumOfComponents = 0;
    unsigned int            mMaxResourcesNum = 0;
    std::mutex              mLock;




    MfxOmxComponentManager();
    MfxOmxComponentManager(const MfxOmxComponentManager&) = delete;
    MfxOmxComponentManager& operator=(const MfxOmxComponentManager& rhs) = delete;
};

#endif /* __MFX_OMX_RESOURCE_MANAGER_H__ */

#endif //#ifdef MFX_RESOURCES_LIMIT
